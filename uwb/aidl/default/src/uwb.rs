use android_hardware_uwb::aidl::android::hardware::uwb::{IUwb, IUwbChip};
use android_hardware_uwb::binder;
use binder::{Result, Strong};

use crate::uwb_chip;

pub struct Uwb {
    chips: Vec<Strong<dyn IUwbChip::IUwbChip>>,
}

impl Uwb {
    pub fn from(chips: impl IntoIterator<Item = uwb_chip::UwbChip>) -> Self {
        Self {
            chips: chips
                .into_iter()
                .map(|chip| {
                    IUwbChip::BnUwbChip::new_binder(chip, binder::BinderFeatures::default())
                })
                .collect(),
        }
    }
}

impl binder::Interface for Uwb {}

impl IUwb::IUwb for Uwb {
    fn getChips(&self) -> Result<Vec<String>> {
        log::info!("getChips");
        self.chips.iter().map(|chip| chip.getName()).collect()
    }

    fn getChip(&self, name: &str) -> Result<Strong<dyn IUwbChip::IUwbChip>> {
        log::info!("getChip {}", name);
        let chip = self
            .chips
            .iter()
            .find(|chip| chip.getName().as_deref() == Ok(name));
        if let Some(chip) = chip {
            Ok(chip.clone())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_ARGUMENT.into())
        }
    }
}
