/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <linux/bpf.h>
#include <linux/netlink.h>
#include <linux/pkt_cls.h>
#include "bpf_helpers.h"

SEC("skfilter")
int filter_netlink(struct __sk_buff* skb) {
    (void)skb;
    return TC_ACT_OK;
}

char _license[] SEC("license") = "Apache 2.0";
