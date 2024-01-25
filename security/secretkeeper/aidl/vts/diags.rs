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

//! Diagnostic display of DICE information.

use chrono::{DateTime, NaiveDateTime, Utc};
use ciborium::value::{Integer, Value};
use coset::{
    cwt::ClaimName, iana, Algorithm, AsCborValue, CborSerializable, CoseKey, CoseSign1,
    KeyOperation, KeyType, Label,
};
use dice_policy::{Constraint, DicePolicy, NodeConstraints};
use diced_open_dice::{DiceArtifacts, OwnedDiceArtifacts};

/// Generate diagnostic output for a simple type.
pub trait SimpleDiagnostic {
    /// Emit a single string describing the item.
    fn diag(&self) -> String;
}

/// Generate diagnostic output for a data structure with inner contents.
pub trait Diagnostic {
    /// Emit a set of strings that describe the item.  Each string should *not*
    /// end with a newline, and lines describing inner content should be indented
    /// by `indent`.
    fn diagnostic(&self, indent: Indent) -> Vec<String>;

    /// As `diagnostic()`, but also indent each line by `outer_indent`.
    fn diagnostic_indented(&self, contents_indent: Indent, outer_indent: Indent) -> Vec<String> {
        prepend_indent(&self.diagnostic(contents_indent), outer_indent)
    }
}

/// Add the given indent before each diagnostic element.
pub fn prepend_indent(diags: &[String], indent: Indent) -> Vec<String> {
    diags.iter().map(|l| format!("{indent}{l}")).collect::<Vec<_>>()
}

/// Number of indent amounts
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub struct Indent(pub usize);

impl std::fmt::Display for Indent {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        // Two space indent per level.
        write!(f, "{}", "  ".repeat(self.0))
    }
}

impl std::ops::Add<Indent> for Indent {
    type Output = Self;
    fn add(self, other: Indent) -> Self {
        Self(self.0 + other.0)
    }
}
impl std::ops::Sub<Indent> for Indent {
    type Output = Self;
    fn sub(self, other: Indent) -> Self {
        Self(self.0 - other.0)
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

// Implementations for basic types and enums.

impl SimpleDiagnostic for ciborium::value::Integer {
    fn diag(&self) -> String {
        match <Self as TryInto<i64>>::try_into(self.clone()) {
            Ok(i) => format!("{i}"),
            Err(_) => format!("{self:?}"),
        }
    }
}

impl SimpleDiagnostic for Value {
    fn diag(&self) -> String {
        match self {
            Value::Integer(i) => format!("{}", i.diag()),
            Value::Text(t) => format!("'{t}'"),
            Value::Bytes(b) => hex::encode(b),
            _ => format!("{self:?}"),
        }
    }
}

impl SimpleDiagnostic for Label {
    fn diag(&self) -> String {
        match self {
            Label::Int(i) => format!("{i}"),
            Label::Text(t) => format!(r#""{t}""#),
        }
    }
}

impl SimpleDiagnostic for KeyType {
    fn diag(&self) -> String {
        match self {
            KeyType::Assigned(iana::KeyType::OKP) => "OKP".to_string(),
            KeyType::Assigned(iana::KeyType::EC2) => "EC2".to_string(),
            KeyType::Assigned(iana::KeyType::RSA) => "RSA".to_string(),
            KeyType::Assigned(iana::KeyType::Symmetric) => "Symmetric".to_string(),
            _ => format!("{self:?}"),
        }
    }
}

impl SimpleDiagnostic for KeyOperation {
    fn diag(&self) -> String {
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

impl SimpleDiagnostic for Algorithm {
    fn diag(&self) -> String {
        match self {
            Algorithm::Assigned(iana::Algorithm::EdDSA) => "EdDSA".to_string(),
            Algorithm::Assigned(iana::Algorithm::ES256) => "ES256".to_string(),
            Algorithm::Assigned(iana::Algorithm::ES384) => "ES384".to_string(),
            Algorithm::Assigned(iana::Algorithm::ES512) => "ES512".to_string(),
            _ => format!("{self:?}"),
        }
    }
}

// Equivalents of `SimpleDiagnostic` for things that don't have their own type.

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
        crate::COMPONENT_RESETTABLE => "resettable".to_string(),
        crate::SECURITY_VERSION => "security_version".to_string(),
        crate::RKP_VM => "rkp_vm".to_string(),
        crate::PAYLOAD_CONFIG_FILENAME => "payload_config_filename".to_string(),
        crate::PAYLOAD_CONFIG => "payload_config".to_string(),
        crate::SUBCOMPONENT_DESCRIPTORS => "subcomponent_descriptors".to_string(),
        v => format!("{v:?}"),
    }
}

// Trait implementations for `coset` structures.

impl Diagnostic for coset::Header {
    fn diagnostic(&self, indent: Indent) -> Vec<String> {
        let mut s = vec!["Header {".to_string()];
        if let Some(alg) = &self.alg {
            s.push(format!("{indent}alg: {},", alg.diag()));
        }
        for crit in &self.crit {
            s.push(format!("{indent}crit: {crit:?},"));
        }
        if let Some(content_type) = &self.content_type {
            let content = match content_type {
                coset::ContentType::Assigned(l) => format!("{l:?}"),
                coset::ContentType::Text(t) => format!("{t}"),
            };
            s.push(format!("{indent}content_type: {},", content));
        }
        if !self.key_id.is_empty() {
            s.push(format!("{indent}kid: {},", hex::encode(&self.key_id)));
        }
        if !self.iv.is_empty() {
            s.push(format!("{indent}iv: {},", hex::encode(&self.iv)));
        }
        if !self.partial_iv.is_empty() {
            s.push(format!("{indent}iv: {},", hex::encode(&self.partial_iv)));
        }
        for sig in &self.counter_signatures {
            s.push(format!("{indent}counter_sig: {sig:?},"));
        }
        for (l, v) in &self.rest {
            s.push(format!("{indent}{}={},", l.diag(), v.diag()));
        }

        s.push("}".to_string());
        s
    }
}

impl Diagnostic for CoseKey {
    fn diagnostic(&self, indent: Indent) -> Vec<String> {
        let mut s = vec!["COSE_Key {".to_string()];
        s.push(format!("{indent}kty: {},", self.kty.diag()));
        let is_ec2 = self.kty == KeyType::Assigned(iana::KeyType::EC2);
        let is_okp = self.kty == KeyType::Assigned(iana::KeyType::OKP);
        if !self.key_id.is_empty() {
            s.push(format!("{indent}kid: {},", hex::encode(&self.key_id)));
        }
        if let Some(alg) = &self.alg {
            s.push(format!("{indent}alg: {},", alg.diag()));
        }
        if !self.key_ops.is_empty() {
            s.push(format!(
                "{indent}ops: {},",
                self.key_ops.iter().map(|op| op.diag()).collect::<Vec<_>>().join(",")
            ));
        }
        if !self.base_iv.is_empty() {
            s.push(format!("{indent}base_iv: {},", hex::encode(&self.base_iv)));
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
                (l, v) => (l.diag(), v.diag()),
            };
            s.push(format!("{indent}{l}: {v},"));
        }
        s.push("}".to_string());
        s
    }
}

impl SimpleDiagnostic for coset::cwt::Timestamp {
    fn diag(&self) -> String {
        let (secs, nsecs) = match self {
            Self::WholeSeconds(i) => (*i, 0),
            Self::FractionalSeconds(f) => (f.trunc() as i64, (1_000_000_000.0 * f.fract()) as u32),
        };
        let dt = DateTime::<Utc>::from_utc(NaiveDateTime::from_timestamp(secs, nsecs), Utc);
        format!("{dt}")
    }
}

impl Diagnostic for coset::cwt::ClaimsSet {
    fn diagnostic(&self, indent: Indent) -> Vec<String> {
        let mut s = vec!["ClaimsSet {".to_string()];
        if let Some(issuer) = &self.issuer {
            s.push(format!("{indent}issuer: \"{}\",", issuer));
        }
        if let Some(subject) = &self.subject {
            s.push(format!("{indent}subject: \"{}\",", subject));
        }
        if let Some(audience) = &self.audience {
            s.push(format!("{indent}audience: \"{}\",", audience));
        }
        if let Some(expiration_time) = &self.expiration_time {
            s.push(format!("{indent}expiration_time: \"{}\",", expiration_time.diag()));
        }
        if let Some(not_before) = &self.not_before {
            s.push(format!("{indent}not_before: \"{}\",", not_before.diag()));
        }
        if let Some(issued_at) = &self.issued_at {
            s.push(format!("{indent}issued_at: \"{}\",", issued_at.diag()));
        }
        if let Some(cwt_id) = &self.cwt_id {
            s.push(format!("{indent}cwt_id: \"{}\",", hex::encode(cwt_id)));
        }
        for (label, value) in &self.rest {
            let mut after = None;
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
                        after = Some(diagnostic_config(&map, indent));
                        ("config_desc".to_string(), "".to_string())
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
                        after = Some(key.diagnostic(indent));
                        ("subject_public_key".to_string(), "".to_string())
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
                (l, v) => (format!("{l:?}"), v.diag()),
            };

            if let Some(after) = &after {
                s.push(format!("{indent}{l}: {v}"));
                s.extend_from_slice(&prepend_indent(after, indent + 1))
            } else {
                s.push(format!("{indent}{l}: {v},"));
            }
        }
        s.push("}".to_string());
        s
    }
}

// Trait implementations for DICE types.

impl Diagnostic for OwnedDiceArtifacts {
    fn diagnostic(&self, indent: Indent) -> Vec<String> {
        let mut s = vec!["OwnedDiceArtifacts {".to_string()];
        s.push(format!("{indent}cdi {{"));
        s.push(format!("{indent}{indent}attest: {},", hex::encode(self.cdi_attest())));
        s.push(format!("{indent}{indent}seal: {},", hex::encode(self.cdi_seal())));
        s.push(format!("{indent}}},"));
        if let Some(bcc) = self.bcc() {
            s.push(format!("{indent}bcc {{"));
            match diagnostic_bcc(bcc, indent) {
                Ok(val) => s.extend_from_slice(&prepend_indent(&val, indent + 1)),
                Err(_) => s.push(format!("{indent}{indent}failed to parse {}", hex::encode(bcc))),
            }
        }
        s.push(format!("{indent}}}"));
        s.push("}".to_string());
        s
    }
}

pub fn diagnostic_bcc(bcc: &[u8], indent: Indent) -> Result<Vec<String>, &'static str> {
    let bcc = Value::from_slice(bcc).map_err(|_e| "cbor deserialize failed")?;
    let bcc = bcc.as_array().ok_or("not an array")?;
    if bcc.is_empty() {
        return Ok(vec!["[]".to_string()]);
    }
    let mut s = vec!["[".to_string()];
    let mut idx = 0;
    if let Some(version) = bcc[0].as_integer() {
        s.push(format!("{indent}version: {},", version.diag()));
        idx += 1;
    }
    if idx >= bcc.len() {
        return Ok(s);
    }

    // First thing should be a COSE_Key.
    if let Ok(uds_pub) = CoseKey::from_cbor_value(bcc[idx].clone()) {
        s.push(format!("{indent}uds_pub:"));
        s.extend_from_slice(&uds_pub.diagnostic_indented(indent, indent + 1));
    } else {
        s.push(format!("{indent}uds_pub: failed to parse {:?}", bcc[idx]));
    }
    idx += 1;

    for cwt in &bcc[idx..] {
        // Subsequent things should be signed CWTs.
        if let Ok(sign) = CoseSign1::from_cbor_value(cwt.clone()) {
            s.push(format!("{indent}chain_entry: CoseSign1 {{"));
            s.push(format!("{indent}{indent}protected:"));
            s.extend_from_slice(&sign.protected.header.diagnostic_indented(indent, indent + 2));
            if let Some(payload) = &sign.payload {
                s.push(format!("{indent}{indent}payload:"));
                if let Ok(cwt) = coset::cwt::ClaimsSet::from_slice(payload) {
                    s.extend_from_slice(&cwt.diagnostic_indented(indent, indent + 2));
                } else {
                    s.push(format!("{indent}{indent}{indent}{}", hex::encode(&payload)));
                }
            }
            s.push(format!("{indent}{indent}signature: {}", hex::encode(&sign.signature)));
            s.push(format!("{indent}}}"));
        } else {
            s.push(format!("{indent}chain_entry: failed to parse {cwt:?}"));
        }
    }
    s.push("]".to_string());
    Ok(s)
}

fn diagnostic_config(map: &[(Value, Value)], indent: Indent) -> Vec<String> {
    let mut s = vec!["{".to_string()];
    for (k, v) in map {
        if let Value::Integer(k) = k {
            if let Ok(k) = <Integer as std::convert::TryInto<i64>>::try_into(k.clone()) {
                let mut after = None;
                let (l, v) = match (k, v) {
                    (crate::COMPONENT_NAME, Value::Text(t)) => {
                        ("component_name".to_string(), format!("{t:?}"))
                    }
                    (crate::COMPONENT_VERSION, Value::Text(t)) => {
                        ("component_version".to_string(), t.to_string())
                    }
                    (crate::COMPONENT_VERSION, Value::Integer(i)) => {
                        ("component_version".to_string(), i.diag())
                    }
                    (crate::COMPONENT_RESETTABLE, Value::Null) => {
                        ("resettable".to_string(), "true".to_string())
                    }
                    (crate::SECURITY_VERSION, Value::Integer(i)) => {
                        ("security_version".to_string(), i.diag())
                    }
                    (crate::RKP_VM, Value::Null) => ("rkp_vm".to_string(), "true".to_string()),
                    (crate::PAYLOAD_CONFIG_FILENAME, Value::Text(t)) => {
                        ("payload_config_filename".to_string(), format!("'{t}'"))
                    }
                    (crate::PAYLOAD_CONFIG, Value::Map(m)) => {
                        let mut val = "{{".to_string();
                        let mut first = true;
                        for (k, v) in m {
                            if !first {
                                val += ", ";
                            }
                            match (k, v) {
                                (Value::Integer(i), Value::Text(t)) if *i == Integer::from(1) => {
                                    val += &format!("path:'{t}'")
                                }
                                (k, v) => val += &format!("{}:{}", k.diag(), v.diag()),
                            }
                            first = false;
                        }

                        val += &format!("}}");
                        ("payload_config_filename".to_string(), val)
                    }
                    (crate::SUBCOMPONENT_DESCRIPTORS, Value::Array(a)) => {
                        let mut val = vec!["[".to_string()];
                        for sub in a {
                            val.extend_from_slice(&prepend_indent(
                                &diagnostic_subcomponent(sub, indent),
                                Indent(1),
                            ));
                        }
                        val.push("]".to_string());
                        after = Some(val);
                        ("subcomponent_descriptors".to_string(), "".to_string())
                    }

                    (l, v) => (format!("{l:?}"), format!("{v:?}")),
                };
                if let Some(after) = &after {
                    s.push(format!("{indent}{l}: {v}"));
                    s.extend_from_slice(&prepend_indent(&after, indent + 1));
                } else {
                    s.push(format!("{indent}{l}: {v},"));
                }
            } else {
                s.push(format!("{indent}{k:?}: {v:?},"));
            }
        } else {
            s.push(format!("{indent}{k:?}: {v:?},"));
        }
    }
    s.push("}".to_string());
    s
}

impl Diagnostic for DicePolicy {
    fn diagnostic(&self, indent: Indent) -> Vec<String> {
        let mut s = vec!["DicePolicy {".to_string()];
        s.push(format!("{indent}version: {},", self.version));
        for node in self.node_constraints_list.iter() {
            s.push(format!("{indent}node_constraint_list:"));
            s.extend_from_slice(&node.diagnostic_indented(indent, indent + 1));
        }
        s.push("}".to_string());
        s
    }
}

impl Diagnostic for NodeConstraints {
    fn diagnostic(&self, indent: Indent) -> Vec<String> {
        let mut s = vec!["NodeConstraints [".to_string()];
        for constraint in self.0.iter() {
            s.extend_from_slice(&constraint.diagnostic_indented(indent, indent));
        }
        s.push("]".to_string());
        s
    }
}

impl Diagnostic for Constraint {
    fn diagnostic(&self, indent: Indent) -> Vec<String> {
        let typestr = match self.constraint_type() {
            dice_policy::EXACT_MATCH_CONSTRAINT => "ExactMatch".to_string(),
            dice_policy::GREATER_OR_EQUAL_CONSTRAINT => "GreaterOrEqual".to_string(),
            t => format!("{t}"),
        };
        let path: Vec<String> = self.path().iter().map(|claim| diag_claim(*claim)).collect();
        let mut s = vec!["Constraint {".to_string()];
        s.push(format!("{indent}type={typestr},"));
        s.push(format!("{indent}path=[{}]", path.join(",")));
        s.push(format!("{indent}value={},", self.value().diag()));
        s.push("},".to_string());
        s
    }
}

fn diagnostic_subcomponent(sub: &Value, indent: Indent) -> Vec<String> {
    let mut s = vec![];
    match sub {
        Value::Map(m) => {
            s.push("{".to_string());
            for (k, v) in m {
                s.push(match (k, v) {
                    (Value::Integer(i), Value::Text(t))
                        if *i == Integer::from(crate::SUBCOMPONENT_NAME) =>
                    {
                        format!("{indent}name: '{t}',")
                    }
                    (Value::Integer(i), Value::Integer(v))
                        if *i == Integer::from(crate::SUBCOMPONENT_SECURITY_VERSION) =>
                    {
                        format!("{indent}security_version: {},", v.diag())
                    }
                    (Value::Integer(i), Value::Bytes(b))
                        if *i == Integer::from(crate::SUBCOMPONENT_CODE_HASH) =>
                    {
                        format!("{indent}code_hash: {},", hex::encode(b))
                    }
                    (Value::Integer(i), Value::Bytes(b))
                        if *i == Integer::from(crate::SUBCOMPONENT_AUTHORITY_HASH) =>
                    {
                        format!("{indent}authority_hash: {},", hex::encode(b))
                    }
                    (k, v) => format!("{}: {},", k.diag(), v.diag()),
                });
            }
            s.push("}".to_string());
        }
        v => s.push(v.diag()),
    }
    s
}
