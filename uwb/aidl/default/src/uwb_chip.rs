use android_hardware_uwb::aidl::android::hardware::uwb::{
    IUwbChip::IUwbChip, IUwbClientCallback::IUwbClientCallback, UwbEvent::UwbEvent,
    UwbStatus::UwbStatus,
};
use android_hardware_uwb::binder;
use binder::{Result, Strong};

use tokio::io::AsyncReadExt;

use std::io::Write;
use std::sync::Mutex;

use crate::serial;

enum State {
    Closed,
    Opened {
        callbacks: Strong<dyn IUwbClientCallback>,
        handle: tokio::runtime::Handle,
        read_task_handle: tokio::task::JoinHandle<()>,
        write: serial::WriteHalf,
    },
}

impl Drop for State {
    fn drop(&mut self) {
        match self {
            State::Closed => {}
            State::Opened {
                handle,
                read_task_handle,
                ..
            } => {
                read_task_handle.abort();

                let result = handle.block_on(read_task_handle);
                assert!(result.unwrap_err().is_cancelled());
            }
        }
    }
}

pub struct UwbChip {
    name: String,
    path: String,
    handle: tokio::runtime::Handle,
    state: Mutex<State>,
}

impl UwbChip {
    pub fn new(name: String, path: String, handle: tokio::runtime::Handle) -> Self {
        Self {
            name,
            path,
            handle,
            state: Mutex::new(State::Closed),
        }
    }
}

impl binder::Interface for UwbChip {}

impl IUwbChip for UwbChip {
    fn getName(&self) -> Result<String> {
        Ok(self.name.clone())
    }

    fn open(&self, callbacks: &Strong<dyn IUwbClientCallback>) -> Result<()> {
        log::info!("open");

        let mut state = self.state.lock().unwrap();

        if let State::Closed = *state {
            let (mut read, write) = serial::open(&self.path, &self.handle)
                .map_err(|_| binder::StatusCode::UNKNOWN_ERROR)?;

            let client_callbacks = callbacks.clone();

            let read_task_handle = self.handle.spawn(async move {
                loop {
                    const UWB_HEADER_SIZE: usize = 4;

                    let mut buffer = vec![0; UWB_HEADER_SIZE];
                    read.read_exact(&mut buffer[0..UWB_HEADER_SIZE])
                        .await
                        .unwrap();

                    let length = buffer[3] as usize + UWB_HEADER_SIZE;

                    buffer.resize(length, 0);
                    read.read_exact(&mut buffer[UWB_HEADER_SIZE..length])
                        .await
                        .unwrap();

                    client_callbacks.onUciMessage(&buffer[..]).unwrap();
                }
            });

            callbacks.onHalEvent(UwbEvent::OPEN_CPLT, UwbStatus::OK)?;

            *state = State::Opened {
                handle: self.handle.clone(),
                callbacks: callbacks.clone(),
                read_task_handle,
                write,
            };

            Ok(())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }

    fn close(&self) -> Result<()> {
        log::info!("close");

        let mut state = self.state.lock().unwrap();

        if let State::Opened { ref callbacks, .. } = *state {
            callbacks.onHalEvent(UwbEvent::CLOSE_CPLT, UwbStatus::OK)?;
            *state = State::Closed;
            Ok(())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }

    fn coreInit(&self) -> Result<()> {
        log::info!("coreInit");

        if let State::Opened { ref callbacks, .. } = *self.state.lock().unwrap() {
            callbacks.onHalEvent(UwbEvent::POST_INIT_CPLT, UwbStatus::OK)?;
            Ok(())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }

    fn sessionInit(&self, _id: i32) -> Result<()> {
        log::info!("sessionInit");

        Ok(())
    }

    fn getSupportedAndroidUciVersion(&self) -> Result<i32> {
        Ok(1)
    }

    fn sendUciMessage(&self, data: &[u8]) -> Result<i32> {
        log::info!("sendUciMessage");

        if let State::Opened { write, .. } = &mut *self.state.lock().unwrap() {
            write
                .write(data)
                .map(|written| written as i32)
                .map_err(|_| binder::StatusCode::UNKNOWN_ERROR.into())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }
}
