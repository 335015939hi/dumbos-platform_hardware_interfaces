/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! HwCryptoOperations tests.

use android_hardware_security_see_hwcrypto::aidl::android::hardware::security::see::hwcrypto::types::{
    AesKey::AesKey, ExplicitKeyMaterial::ExplicitKeyMaterial, KeyType::KeyType, KeyLifetime::KeyLifetime,
    KeyUse::KeyUse, OperationData::OperationData,
    SymmetricOperationParameters::SymmetricOperationParameters, SymmetricOperation::SymmetricOperation,
    CipherModeParameters::CipherModeParameters, AesCipherMode::AesCipherMode, SymmetricCryptoParameters::SymmetricCryptoParameters,
};
use android_hardware_security_see_hwcrypto::aidl::android::hardware::security::see::hwcrypto::{
    KeyPolicy::KeyPolicy,CryptoOperation::CryptoOperation,CryptoOperationSet::CryptoOperationSet,
    OperationParameters::OperationParameters,
};

#[test]
fn test_hwcrypto_key_operations_connection() {
    let hw_crypto_key = hwcryptohal_vts_test::get_hwcryptokey()
        .expect("Couldn't get back a hwcryptokey binder object");
    let hw_crypto_operations = hw_crypto_key.getHwCryptoOperations();
    assert!(hw_crypto_operations.is_ok(), "Couldn't get back a hwcrypto operations binder object");
}

#[test]
fn test_hwcrypto_key_operations_simple_aes_test() {
    let hw_crypto_key = hwcryptohal_vts_test::get_hwcryptokey()
        .expect("Couldn't get back a hwcryptokey binder object");
    let hw_crypto_operations = hw_crypto_key
        .getHwCryptoOperations()
        .expect("Couldn't get back a hwcryptokey operations binder object");
    let clear_key = ExplicitKeyMaterial::Aes(AesKey::Aes128([0; 16]));
    let policy = KeyPolicy {
        usage: KeyUse::ENCRYPT_DECRYPT,
        keyLifetime: KeyLifetime::PORTABLE,
        keyPermissions: Vec::new(),
        keyManagementKey: false,
        keyType: KeyType::AES_128_CBC_PKCS7_PADDING,
    };
    let key = hw_crypto_key.importClearKey(&clear_key, &policy).expect("couldn't import clear key");

    let nonce = [0u8; 16];
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::ENCRYPT;
    let sym_op_params =
        SymmetricOperationParameters { key: Some(key.clone()), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    let input_data = OperationData::DataBuffer("string to be encrypted".as_bytes().to_vec());
    cmd_list.push(CryptoOperation::DataInput(input_data));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    let mut op_result = hw_crypto_operations
        .processCommandList(&mut crypto_sets)
        .expect("couldn't process commands");
    // Extracting the vector from the command list because of ownership
    let CryptoOperation::DataOutput(OperationData::DataBuffer(encrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };
    let context = op_result.remove(0).context;
    // Separating the finish call on a different command set to test the returned context
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(encrypted_data);
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");
    let CryptoOperation::DataOutput(OperationData::DataBuffer(encrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };

    // Decrypting
    let parameters = SymmetricCryptoParameters::Aes(AesCipherMode::Cbc(CipherModeParameters {
        nonce: nonce.into(),
    }));
    let direction = SymmetricOperation::DECRYPT;
    let sym_op_params = SymmetricOperationParameters { key: Some(key), direction, parameters };
    let op_params = OperationParameters::SymmetricCrypto(sym_op_params);
    let mut cmd_list = Vec::<CryptoOperation>::new();
    let data_output = OperationData::DataBuffer(Vec::new());
    cmd_list.push(CryptoOperation::DataOutput(data_output));
    cmd_list.push(CryptoOperation::SetOperationParameters(op_params));
    cmd_list.push(CryptoOperation::DataInput(OperationData::DataBuffer(encrypted_data)));
    cmd_list.push(CryptoOperation::Finish(None));
    let crypto_op_set = CryptoOperationSet { context: None, operations: cmd_list };
    let mut crypto_sets = Vec::new();
    crypto_sets.push(crypto_op_set);
    hw_crypto_operations.processCommandList(&mut crypto_sets).expect("couldn't process commands");
    // Extracting the vector from the command list because of ownership
    let CryptoOperation::DataOutput(OperationData::DataBuffer(decrypted_data)) =
        crypto_sets.remove(0).operations.remove(0)
    else {
        panic!("not reachable, we created this object above on the test");
    };
    let decrypted_msg =
        String::from_utf8(decrypted_data).expect("couldn't decode received message");
    assert_eq!(decrypted_msg, "string to be encrypted", "couldn't retrieve original message");
}
