use android_hardware_uwb::aidl::android::hardware::uwb::{
    IUwbChip::IUwbChipAsyncServer, IUwbClientCallback::IUwbClientCallback, UwbEvent::UwbEvent,
    UwbStatus::UwbStatus,
};
use android_hardware_uwb::binder;
use async_trait::async_trait;
use binder::{Result, Strong, DeathRecipient, IBinder};

use bytes::BytesMut;
use tokio::fs::{File, OpenOptions};
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio_util::sync::CancellationToken;
use tokio::select;
use tokio::sync::Mutex;
use std::sync::Arc;
use log::info;

use std::os::fd::AsRawFd;

use std::io;

use nix::sys::termios;

enum State {
    Closed,
    Opened {
        callbacks: Strong<dyn IUwbClientCallback>,
        #[allow(dead_code)]
        // tasks: tokio::task::JoinSet<()>,
        handle: tokio::task::JoinHandle<()>,
        serial: File,
        death_recipient: DeathRecipient,
        token: CancellationToken,
    },
}

pub struct UwbChip {
    name: String,
    path: String,
    state: Arc<Mutex<State>>,
}

impl UwbChip {
    pub fn new(name: String, path: String) -> Self {
        Self {
            name,
            path,
            state: Arc::new(Mutex::new(State::Closed)),
        }
    }
}

pub fn makeraw(file: File) -> io::Result<File> {
    let fd = file.as_raw_fd();

    let mut attrs = termios::tcgetattr(fd)?;

    termios::cfmakeraw(&mut attrs);

    termios::tcsetattr(fd, termios::SetArg::TCSANOW, &attrs)?;

    Ok(file)
}

impl binder::Interface for UwbChip {}

#[async_trait]
impl IUwbChipAsyncServer for UwbChip {
    async fn getName(&self) -> Result<String> {
        Ok(self.name.clone())
    }

    async fn open(&self, callbacks: &Strong<dyn IUwbClientCallback>) -> Result<()> {
        log::debug!("open: {:?}", &self.path);

        let mut state = self.state.lock().await;
        if let State::Opened { ref callbacks, ref mut death_recipient, ref mut handle, .. } = *state {
            callbacks.as_binder().unlink_to_death(death_recipient)?;
            callbacks.onHalEvent(UwbEvent::CLOSE_CPLT, UwbStatus::OK)?;
            // tasks.shutdown().await;
            let _ = handle.await;
            *state = State::Closed;
        }

        let serial = OpenOptions::new()
            .read(true)
            .write(true)
            .create(false)
            .open(&self.path)
            .await
            .and_then(makeraw)
            .map_err(|_| binder::StatusCode::UNKNOWN_ERROR)?;

        let status_death_recipient = self.state.clone();
        let mut death_recipient = DeathRecipient::new(move || {
            let mut status = status_death_recipient.blocking_lock();
            if let State::Opened { callbacks: _, handle: _, serial: _, death_recipient: _, token: _ } = *status {
                log::info!("Uwb service has died");
                *status = State::Closed;
            }
        });
        callbacks.as_binder().link_to_death(&mut death_recipient)?;

        let token = CancellationToken::new();
        let cloned_token = token.clone();

        if let State::Closed = *state {
            let client_callbacks = callbacks.clone();

            // let mut tasks = tokio::task::JoinSet::new();
            let mut reader = serial
                .try_clone()
                .await
                .map_err(|_| binder::StatusCode::UNKNOWN_ERROR)?;

            // let state_read_task = self.state.clone();

            let join_handle = tokio::task::spawn(async move {
                
                'outer: loop {
                    let mut buffer = BytesMut::new();
                    const UWB_HEADER_SIZE: usize = 4;
                    info!("buffer0: {:?}", buffer.clone().to_vec());
                    // let mut buffer = vec![0; UWB_HEADER_SIZE];
                    let mut header_buf = vec![0; UWB_HEADER_SIZE];
                    let mut already_read = 0;

                    loop {
                        select! {
                            _ = cloned_token.cancelled() => {
                                    
                                    if already_read != 0 {
                                        reader.write_all(&header_buf[..already_read]).await.unwrap();
                                        reader.flush().await.unwrap();
                                    }
                                    info!("task is cancelled!");
                                    break 'outer;
                                },
                            res = reader.read(&mut header_buf[already_read..]) => {
                                match res {
                                    Ok(buf_len) => {
                                        already_read += buf_len;
                                        if already_read == UWB_HEADER_SIZE {
                                            break;
                                        }
                                    }
                                    Err(_) => panic!(),
                                }
                            }
                        }
                        // let buf_len = reader.read(&mut header_buf[already_read..]).await.unwrap();
                    }
                    buffer.extend_from_slice(&header_buf);
                    
                    
                    // select! {
                    //     _ = cloned_token.cancelled() => {
                    //         info!("task is cancelled!");
                    //         break;
                    //     },
                    //     res = reader.read_exact(&mut buffer[0..UWB_HEADER_SIZE]) => {
                    //         match res {
                    //             Ok(size) => info!("read size: {:?}", size),
                    //             Err(_) => panic!(),
                    //         }
                    //     }
                    // }
                    // reader
                    //     .read(&mut buffer[0..UWB_HEADER_SIZE])
                    //     .await
                    //     .unwrap();
                    info!("buffer1: {:?}", buffer.clone().to_vec());
                    let length = buffer[3] as usize + UWB_HEADER_SIZE;

                    buffer.resize(length, 0);
                    info!("buffer2: {:?}", buffer.clone().to_vec());
                    reader
                        .read_exact(&mut buffer[UWB_HEADER_SIZE..length])
                        .await
                        .unwrap();

                    info!("buffer3: {:?}", buffer.clone().to_vec());
                    client_callbacks.onUciMessage(&buffer[..]).unwrap();
                }
            });

            callbacks.onHalEvent(UwbEvent::OPEN_CPLT, UwbStatus::OK)?;

            *state = State::Opened {
                callbacks: callbacks.clone(),
                handle: join_handle,
                serial,
                death_recipient,
                token,
            };

            Ok(())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }

    async fn close(&self) -> Result<()> {
        log::debug!("close");

        let mut state = self.state.lock().await;

        if let State::Opened { ref callbacks, ref mut death_recipient, ref mut handle, ref mut token,.. } = *state {
            token.cancel();
            callbacks.as_binder().unlink_to_death(death_recipient)?;
            callbacks.onHalEvent(UwbEvent::CLOSE_CPLT, UwbStatus::OK)?;
            let _ = handle.await;
            *state = State::Closed;
            Ok(())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }

    async fn coreInit(&self) -> Result<()> {
        log::debug!("coreInit");

        if let State::Opened { ref callbacks, .. } = *self.state.lock().await {
            callbacks.onHalEvent(UwbEvent::POST_INIT_CPLT, UwbStatus::OK)?;
            Ok(())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }

    async fn sessionInit(&self, _id: i32) -> Result<()> {
        log::debug!("sessionInit");

        Ok(())
    }

    async fn getSupportedAndroidUciVersion(&self) -> Result<i32> {
        Ok(1)
    }

    async fn sendUciMessage(&self, data: &[u8]) -> Result<i32> {
        log::debug!("sendUciMessage");

        if let State::Opened { ref mut serial, .. } = &mut *self.state.lock().await {
            serial
                .write(data)
                .await
                .map(|written| written as i32)
                .map_err(|_| binder::StatusCode::UNKNOWN_ERROR.into())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }
}
