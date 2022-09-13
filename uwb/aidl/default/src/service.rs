use android_hardware_uwb::aidl::android::hardware::uwb::IUwb::{self, IUwb as _};
use android_hardware_uwb::binder;

use tokio::runtime::Runtime;

use std::env;
use std::panic;

mod serial;
mod uwb;
mod uwb_chip;

fn main() -> anyhow::Result<()> {
    android_logger::init_once(
        android_logger::Config::default()
            .with_min_level(log::Level::Debug)
            .with_tag("android.hardware.uwb"),
    );

    // Redirect panic messages to logcat.
    panic::set_hook(Box::new(|panic_info| {
        log::error!("{}", panic_info);
    }));

    log::info!("UWB HAL starting up");

    // Create the tokio runtime
    let rt = Runtime::new()?;

    let chips = env::args()
        .skip(1) // Skip binary name
        .enumerate()
        .map(|(i, arg)| uwb_chip::UwbChip::new(i.to_string(), arg, rt.handle().clone()));

    binder::add_service(
        &format!("{}/default", IUwb::BpUwb::get_descriptor()),
        IUwb::BnUwb::new_binder(uwb::Uwb::from(chips), binder::BinderFeatures::default())
            .as_binder(),
    )?;

    binder::ProcessState::join_thread_pool();
    Ok(())
}
