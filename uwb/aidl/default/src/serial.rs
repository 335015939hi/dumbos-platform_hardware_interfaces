use std::io;
use std::pin::Pin;
use std::task::{Context, Poll};

use std::os::unix::io::RawFd;
use tokio::io::unix::AsyncFd;

use nix::sys::{stat, termios};
use nix::{fcntl, unistd};

pub struct ReadHalf(AsyncFd<RawFd>);

impl ReadHalf {
    fn new(fd: RawFd, handle: &tokio::runtime::Handle) -> io::Result<Self> {
        fcntl::fcntl(fd, fcntl::FcntlArg::F_SETFL(fcntl::OFlag::O_NONBLOCK))
            .map_err(|errno| io::Error::from_raw_os_error(errno as i32))?;

        let guard = handle.enter();
        let async_fd = AsyncFd::new(fd)?;
        drop(guard);

        Ok(ReadHalf(async_fd))
    }
}

impl tokio::io::AsyncRead for ReadHalf {
    fn poll_read(
        self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &mut tokio::io::ReadBuf<'_>,
    ) -> Poll<io::Result<()>> {
        loop {
            let mut guard = match self.0.poll_read_ready(cx) {
                Poll::Ready(value) => value,
                Poll::Pending => return Poll::Pending,
            }?;

            match guard.try_io(|inner| {
                unistd::read(*inner.get_ref(), buf.initialize_unfilled())
                    .map_err(|errno| io::Error::from_raw_os_error(errno as i32))
            }) {
                Ok(result) => {
                    buf.advance(result?);
                    return Poll::Ready(Ok(()));
                }
                Err(_would_block) => continue,
            }
        }
    }
}

impl Drop for ReadHalf {
    fn drop(&mut self) {
        let _ = unistd::close(*self.0.get_ref());
    }
}

pub struct WriteHalf(RawFd);

impl io::Write for WriteHalf {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        unistd::write(self.0, buf).map_err(|errno| io::Error::from_raw_os_error(errno as i32))
    }

    fn flush(&mut self) -> io::Result<()> {
        Ok(())
    }
}

impl Drop for WriteHalf {
    fn drop(&mut self) {
        let _ = unistd::close(self.0);
    }
}

pub fn open(path: &str, handle: &tokio::runtime::Handle) -> io::Result<(ReadHalf, WriteHalf)> {
    let file = fcntl::open(path, fcntl::OFlag::O_RDWR, stat::Mode::empty())?;

    let mut attrs = termios::tcgetattr(file)?;

    termios::cfmakeraw(&mut attrs);

    termios::tcsetattr(file, termios::SetArg::TCSANOW, &attrs)?;

    let read = file;
    let write = unistd::dup(read)?;

    Ok((ReadHalf::new(read, handle)?, WriteHalf(write)))
}
