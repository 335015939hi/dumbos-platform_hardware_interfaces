/*
 * Copyright (C) 2023 The Android Open Source Project
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

//! Diagnostic displace of DICE information.

use chrono::{DateTime, NaiveDateTime, Utc};
use ciborium::value::{Integer, Value};
use coset::{
    cwt::ClaimName, iana, Algorithm, AsCborValue, CborSerializable, CoseKey, CoseSign1,
    KeyOperation, KeyType, Label,
};
use dice_policy::{Constraint, DicePolicy, NodeConstraints};
use diced_open_dice::{DiceArtifacts, OwnedDiceArtifacts};

/// Amount to indent.
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub struct Indent(pub usize);

impl std::fmt::Display for Indent {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", " ".repeat(self.0))
    }
}

impl std::ops::Add<usize> for Indent {
    type Output = Self;
    fn add(self, other: usize) -> Self {
        Self(self.0 + other)
    }
}
impl std::ops::Sub<usize> for Indent {
    type Output = Self;
    fn sub(self, other: usize) -> Self {
        Self(self.0 - other)
    }
}

/// Generate diagnostic output for a data structure.
pub trait Diagnostic {
    /// Emit a multi-line string describing the item.
    fn diagnostic(&self) -> String;

    /// Emit a multi-line string describing the item, with each line indented.
    fn diagnostic_indented(&self, indent: Indent) -> String {
        self.diagnostic().lines().map(|l| format!("{indent}{l}\n")).collect::<Vec<_>>().join("")
    }
}

impl Diagnostic for ciborium::value::Integer {
    fn diagnostic(&self) -> String {
        match <Self as TryInto<i64>>::try_into(self.clone()) {
            Ok(i) => format!("{i}"),
            Err(_) => format!("{self:?}"),
        }
    }
}

impl Diagnostic for Value {
    fn diagnostic(&self) -> String {
        match self {
            Value::Integer(i) => format!("{}", i.diagnostic()),
            Value::Bytes(b) => hex::encode(b),
            _ => format!("{self:?}"),
        }
    }
}

impl Diagnostic for Label {
    fn diagnostic(&self) -> String {
        match self {
            Label::Int(i) => format!("{i}"),
            Label::Text(t) => format!(r#""{t}""#),
        }
    }
}

impl Diagnostic for KeyType {
    fn diagnostic(&self) -> String {
        match self {
            KeyType::Assigned(iana::KeyType::OKP) => "OKP".to_string(),
            KeyType::Assigned(iana::KeyType::EC2) => "EC2".to_string(),
            KeyType::Assigned(iana::KeyType::RSA) => "RSA".to_string(),
            KeyType::Assigned(iana::KeyType::Symmetric) => "Symmetric".to_string(),
            _ => format!("{self:?}"),
        }
    }
}

impl Diagnostic for KeyOperation {
    fn diagnostic(&self) -> String {
        match self {
            KeyOperation::Assigned(iana::KeyOperation::Sign) => "sign".to_string(),
            KeyOperation::Assigned(iana::KeyOperation::Verify) => "verify".to_string(),
            KeyOperation::Assigned(iana::KeyOperation::Encrypt) => "encrypt".to_string(),
            KeyOperation::Assigned(iana::KeyOperation::Decrypt) => "decrypt".to_string(),
            KeyOperation::Assigned(iana::KeyOperation::WrapKey) => "wrap".to_string(),
            KeyOperation::Assigned(iana::KeyOperation::UnwrapKey) => "unwrap".to_string(),
            KeyOperation::Assigned(iana::KeyOperation::DeriveKey) => "derive_key".to_string(),
            KeyOperation::Assigned(iana::KeyOperation::DeriveBits) => "derive_bits".to_string(),
            KeyOperation::Assigned(iana::KeyOperation::MacCreate) => "mac_create".to_string(),
            KeyOperation::Assigned(iana::KeyOperation::MacVerify) => "mac_verify".to_string(),
            _ => format!("{self:?}"),
        }
    }
}

impl Diagnostic for Algorithm {
    fn diagnostic(&self) -> String {
        match self {
            Algorithm::Assigned(iana::Algorithm::EdDSA) => "EdDSA".to_string(),
            Algorithm::Assigned(iana::Algorithm::ES256) => "ES256".to_string(),
            Algorithm::Assigned(iana::Algorithm::ES384) => "ES384".to_string(),
            Algorithm::Assigned(iana::Algorithm::ES512) => "ES512".to_string(),
            _ => format!("{self:?}"),
        }
    }
}
impl Diagnostic for coset::Header {
    fn diagnostic(&self) -> String {
        let mut s = format!("Header {{\n");
        if let Some(alg) = &self.alg {
            s += &format!("  alg: {}\n", alg.diagnostic());
        }
        for crit in &self.crit {
            s += &format!("  crit: {crit:?}\n");
        }
        if let Some(content_type) = &self.content_type {
            let content = match content_type {
                coset::ContentType::Assigned(l) => format!("{l:?}"),
                coset::ContentType::Text(t) => format!("{t}"),
            };
            s += &format!("  content_type: {}\n", content);
        }
        if !self.key_id.is_empty() {
            s += &format!("  kid: {}\n", hex::encode(&self.key_id));
        }
        if !self.iv.is_empty() {
            s += &format!("  iv: {}\n", hex::encode(&self.iv));
        }
        if !self.partial_iv.is_empty() {
            s += &format!("  iv: {}\n", hex::encode(&self.partial_iv));
        }
        for sig in &self.counter_signatures {
            s += &format!("  counter_sig: {sig:?}\n");
        }
        for (l, v) in &self.rest {
            s += &format!("  {}={}\n", l.diagnostic(), v.diagnostic());
        }

        s += &format!("}}\n");
        s
    }
}

impl Diagnostic for CoseKey {
    fn diagnostic(&self) -> String {
        let mut s = format!("COSE_Key {{\n");
        s += &format!("  kty: {}\n", self.kty.diagnostic());
        let is_ec2 = self.kty == KeyType::Assigned(iana::KeyType::EC2);
        let is_okp = self.kty == KeyType::Assigned(iana::KeyType::OKP);
        if !self.key_id.is_empty() {
            s += &format!("  kid: {}\n", hex::encode(&self.key_id));
        }
        if let Some(alg) = &self.alg {
            s += &format!("  alg: {}\n", alg.diagnostic());
        }
        if !self.key_ops.is_empty() {
            s += &format!(
                "  ops: {}\n",
                self.key_ops.iter().map(|op| op.diagnostic()).collect::<Vec<_>>().join(",")
            );
        }
        if !self.base_iv.is_empty() {
            s += &format!("  base_iv: {}\n", hex::encode(&self.base_iv));
        }
        for (label, value) in &self.params {
            let (l, v) = match (label, value) {
                // Special case common EC2 parameters.
                (Label::Int(l), Value::Integer(curve))
                    if is_ec2 && *l == iana::Ec2KeyParameter::Crv as i64 =>
                {
                    ("crv".to_string(), diag_curve(&curve))
                }
                (Label::Int(l), Value::Bytes(x))
                    if is_ec2 && *l == iana::Ec2KeyParameter::X as i64 =>
                {
                    ("x".to_string(), hex::encode(&x))
                }
                (Label::Int(l), Value::Bytes(y))
                    if is_ec2 && *l == iana::Ec2KeyParameter::Y as i64 =>
                {
                    ("y".to_string(), hex::encode(&y))
                }

                // Special case common OKP parameters.
                (Label::Int(l), Value::Integer(curve))
                    if is_okp && *l == iana::OkpKeyParameter::Crv as i64 =>
                {
                    ("crv".to_string(), diag_curve(&curve))
                }
                (Label::Int(l), Value::Bytes(x))
                    if is_okp && *l == iana::OkpKeyParameter::X as i64 =>
                {
                    ("x".to_string(), hex::encode(&x))
                }

                // General case: just show the label and value.
                (l, v) => (l.diagnostic(), v.diagnostic()),
            };
            s += &format!("  {l}: {v}\n");
        }
        s += &format!("}}\n");
        s
    }
}

impl Diagnostic for coset::cwt::Timestamp {
    fn diagnostic(&self) -> String {
        let (secs, nsecs) = match self {
            Self::WholeSeconds(i) => (*i, 0),
            Self::FractionalSeconds(f) => (f.trunc() as i64, (1_000_000_000.0 * f.fract()) as u32),
        };
        let dt = DateTime::<Utc>::from_utc(NaiveDateTime::from_timestamp(secs, nsecs), Utc);
        format!("{dt}")
    }
}

impl Diagnostic for coset::cwt::ClaimsSet {
    fn diagnostic(&self) -> String {
        let mut s = format!("ClaimsSet {{\n");
        if let Some(issuer) = &self.issuer {
            s += &format!("  issuer: \"{}\"\n", issuer);
        }
        if let Some(subject) = &self.subject {
            s += &format!("  subject: \"{}\"\n", subject);
        }
        if let Some(audience) = &self.audience {
            s += &format!("  audience: \"{}\"\n", audience);
        }
        if let Some(expiration_time) = &self.expiration_time {
            s += &format!("  expiration_time: \"{}\"\n", expiration_time.diagnostic());
        }
        if let Some(not_before) = &self.not_before {
            s += &format!("  not_before: \"{}\"\n", not_before.diagnostic());
        }
        if let Some(issued_at) = &self.issued_at {
            s += &format!("  issued_at: \"{}\"\n", issued_at.diagnostic());
        }
        if let Some(cwt_id) = &self.cwt_id {
            s += &format!("  cwt_id: \"{}\"\n", hex::encode(cwt_id));
        }
        for (label, value) in &self.rest {
            let (l, v) = match (label, value) {
                (ClaimName::PrivateUse(crate::CODE_HASH), Value::Bytes(b)) => {
                    ("code_hash".to_string(), hex::encode(b))
                }
                (ClaimName::PrivateUse(crate::CODE_DESC), Value::Bytes(b)) => {
                    ("code_desc".to_string(), hex::encode(b))
                }
                (ClaimName::PrivateUse(crate::CONFIG_HASH), Value::Bytes(b)) => {
                    ("config_hash".to_string(), hex::encode(b))
                }
                (ClaimName::PrivateUse(crate::CONFIG_DESC), Value::Bytes(b)) => {
                    if let Ok(Value::Map(map)) = Value::from_slice(b) {
                        ("config_desc".to_string(), format!("\n{}", diag_config(&map, Indent(4))))
                    } else {
                        ("config_desc".to_string(), hex::encode(b))
                    }
                }
                (ClaimName::PrivateUse(crate::AUTHORITY_HASH), Value::Bytes(b)) => {
                    ("authority_hash".to_string(), hex::encode(b))
                }
                (ClaimName::PrivateUse(crate::AUTHORITY_DESCRIPTOR), Value::Bytes(b)) => {
                    ("authority_descriptor".to_string(), hex::encode(b))
                }
                (ClaimName::PrivateUse(crate::MODE), Value::Bytes(b)) => {
                    let val = match &b[..] {
                        [0x00] => "NotConfigured".to_string(),
                        [0x01] => "Normal".to_string(),
                        [0x02] => "Debug".to_string(),
                        [0x03] => "Recovery".to_string(),
                        b => hex::encode(b),
                    };
                    ("mode".to_string(), val)
                }
                (ClaimName::PrivateUse(crate::SUBJECT_PUBLIC_KEY), Value::Bytes(b)) => {
                    if let Ok(key) = CoseKey::from_slice(b) {
                        (
                            "subject_public_key".to_string(),
                            format!("\n{}", key.diagnostic_indented(Indent(4))),
                        )
                    } else {
                        ("subject_public_key".to_string(), hex::encode(b))
                    }
                }
                (ClaimName::PrivateUse(crate::KEY_USAGE), Value::Bytes(b)) => {
                    let val = if b.len() == 1 || b.len() == 2 {
                        let mut uses = Vec::new();
                        if b[0] & 0x80 != 0 {
                            uses.push("digitalSignature");
                        }
                        if b[0] & 0x40 != 0 {
                            uses.push("nonRepudiation");
                        }
                        if b[0] & 0x20 != 0 {
                            uses.push("keyEncipherment");
                        }
                        if b[0] & 0x10 != 0 {
                            uses.push("dataEncipherment");
                        }
                        if b[0] & 0x08 != 0 {
                            uses.push("keyAgreement");
                        }
                        if b[0] & 0x04 != 0 {
                            uses.push("keyCertSign");
                        }
                        if b[0] & 0x02 != 0 {
                            uses.push("cRLSign");
                        }
                        if b[0] & 0x01 != 0 {
                            uses.push("encipherOnly");
                        }
                        if b.len() == 2 && b[1] & 0x80 != 0 {
                            uses.push("decipherOnly");
                        }
                        uses.join(",")
                    } else {
                        hex::encode(b)
                    };
                    ("key_usage".to_string(), val)
                }
                (ClaimName::PrivateUse(crate::PROFILE_NAME), Value::Bytes(b)) => {
                    ("profile_name".to_string(), hex::encode(b))
                }

                // General case: just show the label and value.
                (l, v) => (format!("{l:?}"), v.diagnostic()),
            };

            s += &format!("  {l}: {v}");
            if !v.ends_with("\n") {
                s += &format!("\n");
            }
        }
        s += &format!("}}\n");
        s
    }
}

impl Diagnostic for OwnedDiceArtifacts {
    fn diagnostic(&self) -> String {
        let mut s = format!("OwnedDiceArtifacts {{\n");
        s += &format!("  cdi {{\n");
        s += &format!("    attest: {}\n", hex::encode(self.cdi_attest()));
        s += &format!("    seal: {}\n", hex::encode(self.cdi_seal()));
        s += &format!("  }},\n",);
        if let Some(bcc) = self.bcc() {
            s += &format!("  bcc {{\n");
            match diag_bcc(bcc, Indent(4)) {
                Ok(val) => s += &val,
                Err(_) => s += &format!("    failed to parse {}\n", hex::encode(bcc)),
            }
        }
        s += &format!("  }}\n");
        s += &format!("}}\n");
        s
    }
}

pub fn diag_bcc(bcc: &[u8], indent: Indent) -> Result<String, &'static str> {
    let bcc = Value::from_slice(bcc).map_err(|_e| "cbor deserialize failed")?;
    let bcc = bcc.as_array().ok_or("not an array")?;
    if bcc.is_empty() {
        return Ok(format!("{indent}<empty>"));
    }
    let mut s = String::new();
    let mut idx = 0;
    if let Some(version) = bcc[0].as_integer() {
        s += &format!("{indent}version: {}", version.diagnostic());
        idx += 1;
    }
    if idx >= bcc.len() {
        return Ok(s);
    }

    // First thing should be a COSE_Key.
    if let Ok(uds_pub) = CoseKey::from_cbor_value(bcc[idx].clone()) {
        s += &format!("{indent}uds_pub:\n{}", uds_pub.diagnostic_indented(indent + 2));
    } else {
        s += &format!("{indent}uds_pub: failed to parse {:?}\n", bcc[idx]);
    }
    idx += 1;

    while idx < bcc.len() {
        // Subsequent things should be signed CWTs.
        if let Ok(sign) = CoseSign1::from_cbor_value(bcc[idx].clone()) {
            s += &format!("{indent}chain_entry: CoseSign1 {{\n");
            s += &format!("{indent}  protected:\n");
            s += &format!("{}", sign.protected.header.diagnostic_indented(indent + 4));
            if let Some(payload) = &sign.payload {
                s += &format!("{indent}  payload:\n");
                if let Ok(cwt) = coset::cwt::ClaimsSet::from_slice(payload) {
                    s += &format!("{}", cwt.diagnostic_indented(indent + 4));
                } else {
                    s += &format!("{indent}    {}\n", hex::encode(&payload));
                }
            }
            s += &format!("{indent}  signature: {}\n", hex::encode(&sign.signature));
            s += &format!("{indent}}}\n");
        } else {
            s += &format!("{indent}chain_entry: failed to parse {:?}\n", bcc[idx]);
        }

        idx += 1;
    }

    Ok(s)
}

fn diag_config(map: &[(Value, Value)], indent: Indent) -> String {
    let mut s = format!("{indent}{{\n");

    for (k, v) in map {
        if let Value::Integer(k) = k {
            if let Ok(k) = <Integer as std::convert::TryInto<i64>>::try_into(k.clone()) {
                let (l, v) = match (k, v) {
                    (crate::COMPONENT_NAME, Value::Text(t)) => {
                        ("component_name".to_string(), format!("{t:?}"))
                    }
                    (crate::COMPONENT_VERSION, Value::Text(t)) => {
                        ("component_version".to_string(), t.to_string())
                    }
                    (crate::COMPONENT_VERSION, Value::Integer(i)) => {
                        ("component_version".to_string(), i.diagnostic())
                    }
                    (crate::RESETTABLE, Value::Null) => {
                        ("resettable".to_string(), "true".to_string())
                    }
                    (crate::SECURITY_VERSION, Value::Integer(i)) => {
                        ("security_version".to_string(), i.diagnostic())
                    }
                    (crate::RKP_VM, Value::Null) => ("rkp_vm".to_string(), "true".to_string()),
                    (crate::PAYLOAD_CONFIG_FILENAME, Value::Text(t)) => {
                        ("payload_config_filename".to_string(), format!("'{t}'"))
                    }
                    (crate::PAYLOAD_CONFIG, Value::Map(m)) => {
                        let mut val = format!("{{");
                        let mut first = true;
                        for (k, v) in m {
                            if !first {
                                val += ", ";
                            }
                            match (k, v) {
                                (Value::Integer(i), Value::Text(t)) if *i == Integer::from(1) => {
                                    val += &format!("path:'{t}'")
                                }
                                (k, v) => val += &format!("{}:{}", k.diagnostic(), v.diagnostic()),
                            }
                            first = false;
                        }

                        val += &format!("}}\n");
                        ("payload_config_filename".to_string(), val)
                    }
                    (crate::CONFIG_DESC_SUB_COMPONENTS, Value::Array(a)) => {
                        let mut val = format!("[\n");
                        for sub in a {
                            val += &format!("{indent}    {}", diag_subcomponent(sub));
                        }
                        val += &format!("{indent}  ]");
                        ("subcomponent_descriptors".to_string(), val)
                    }

                    (l, v) => (format!("{l:?}"), format!("{v:?}")),
                };
                s += &format!("{indent}  {l}: {v}\n");
            } else {
                s += &format!("{indent}  {k:?}: {v:?}\n");
            }
        } else {
            s += &format!("{indent}  {k:?}: {v:?}\n");
        }
    }

    s += &format!("{indent}}}\n");
    s
}

fn diag_curve(curve: &Integer) -> String {
    if let Ok(curve) = <Integer as std::convert::TryInto<i64>>::try_into(curve.clone()) {
        match curve {
            x if x == iana::EllipticCurve::P_256 as i64 => "P-256".to_string(),
            x if x == iana::EllipticCurve::P_384 as i64 => "P-384".to_string(),
            x if x == iana::EllipticCurve::P_521 as i64 => "P-521".to_string(),
            x if x == iana::EllipticCurve::X25519 as i64 => "X25519".to_string(),
            x if x == iana::EllipticCurve::X448 as i64 => "X448".to_string(),
            x if x == iana::EllipticCurve::Ed25519 as i64 => "Ed25519".to_string(),
            x if x == iana::EllipticCurve::Ed448 as i64 => "Ed448".to_string(),
            x if x == iana::EllipticCurve::Secp256k1 as i64 => "secp256k1".to_string(),
            c => format!("{c}"),
        }
    } else {
        format!("{curve:?}")
    }
}

fn diag_claim(claim: i64) -> String {
    match claim {
        crate::CODE_HASH => "code_hash".to_string(),
        crate::CODE_DESC => "code_desc".to_string(),
        crate::CONFIG_HASH => "config_hash".to_string(),
        crate::CONFIG_DESC => "config_desc".to_string(),
        crate::AUTHORITY_HASH => "authority_hash".to_string(),
        crate::AUTHORITY_DESCRIPTOR => "authority_descriptor".to_string(),
        crate::MODE => "mode".to_string(),
        crate::SUBJECT_PUBLIC_KEY => "subject_public_key".to_string(),
        crate::KEY_USAGE => "key_usage".to_string(),
        crate::PROFILE_NAME => "profile_name".to_string(),
        crate::COMPONENT_NAME => "component_name".to_string(),
        crate::COMPONENT_VERSION => "component_version".to_string(),
        crate::RESETTABLE => "resettable".to_string(),
        crate::SECURITY_VERSION => "security_version".to_string(),
        crate::RKP_VM => "rkp_vm".to_string(),
        crate::PAYLOAD_CONFIG_FILENAME => "payload_config_filename".to_string(),
        crate::PAYLOAD_CONFIG => "payload_config".to_string(),
        crate::CONFIG_DESC_SUB_COMPONENTS => "subcomponent_descriptors".to_string(),
        v => format!("{v:?}"),
    }
}

impl Diagnostic for DicePolicy {
    fn diagnostic(&self) -> String {
        let mut s = format!("DicePolicy {{\n");
        s += &format!("  version: {}\n", self.version);
        for node in self.node_constraints_list.iter() {
            s += &format!("  node_constraint_list:\n{}", node.diagnostic_indented(Indent(4)));
        }
        s += &format!("}}\n");
        s
    }
}

impl Diagnostic for NodeConstraints {
    fn diagnostic(&self) -> String {
        let mut s = format!("NodeConstraints [\n");
        for constraint in self.0.iter() {
            s += &format!("  {}\n", constraint.diagnostic());
        }
        s += &format!("]\n");
        s
    }
}

impl Diagnostic for Constraint {
    fn diagnostic(&self) -> String {
        let typestr = match self.constraint_type() {
            dice_policy::EXACT_MATCH_CONSTRAINT => "ExactMatch".to_string(),
            dice_policy::GREATER_OR_EQUAL_CONSTRAINT => "GreaterOrEqual".to_string(),
            t => format!("{t}"),
        };
        let path: Vec<String> = self.path().iter().map(|claim| diag_claim(*claim)).collect();
        format!(
            "Constraint {{ type={typestr}, path={path:?}, value={:?} }}",
            self.value().diagnostic()
        )
    }
}

fn diag_subcomponent(sub: &Value) -> String {
    match sub {
        Value::Map(m) => {
            let mut first = true;
            let mut s = format!("{{ ");
            for (k, v) in m {
                if !first {
                    s += ", ";
                }
                s += &match (k, v) {
                    (Value::Integer(i), Value::Text(t)) if *i == Integer::from(1) => {
                        format!("name:'{t}'")
                    }
                    (Value::Integer(i), Value::Integer(v)) if *i == Integer::from(2) => {
                        format!("security_version:{}", v.diagnostic())
                    }
                    (Value::Integer(i), Value::Bytes(b)) if *i == Integer::from(3) => {
                        format!("code_hash:{}", hex::encode(b))
                    }
                    (Value::Integer(i), Value::Bytes(b)) if *i == Integer::from(4) => {
                        format!("authority_hash:{}", hex::encode(b))
                    }
                    (k, v) => format!("{}:{}", k.diagnostic(), v.diagnostic()),
                };
                first = false;
            }
            s += &format!(" }}\n");
            s
        }
        v => v.diagnostic(),
    }
}
