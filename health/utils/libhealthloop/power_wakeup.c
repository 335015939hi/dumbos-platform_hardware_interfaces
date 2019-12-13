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

unsigned long long load_byte(void* skb, unsigned long long off) asm("llvm.bpf.load.byte");
unsigned long long load_word(void* skb, unsigned long long off) asm("llvm.bpf.load.word");

#define M(SKB, OFF, C) (load_byte(SKB, OFF) == C)

#define MATCH_POWER_SUPPLY0_LENGTH 13
#define MATCH_POWER_SUPPLY0(SKB, I)                                                             \
    ((M(SKB, I + 0, 'p') && M(SKB, I + 1, 'o') && M(SKB, I + 2, 'w') && M(SKB, I + 3, 'e') &&   \
      M(SKB, I + 4, 'r') && M(SKB, I + 5, '_') && M(SKB, I + 6, 's') && M(SKB, I + 7, 'u') &&   \
      M(SKB, I + 8, 'p') && M(SKB, I + 9, 'p') && M(SKB, I + 10, 'l') && M(SKB, I + 11, 'y') && \
      M(SKB, I + 12, 0))                                                                        \
             ? TC_ACT_OK                                                                        \
             : TC_ACT_SHOT)

#define MATCH_SUBSYSTEM_LENGTH 10
#define MATCH_SUBSYSTEM(SKB, I)                                                              \
    (M(SKB, I + 0, 'S') && M(SKB, I + 1, 'U') && M(SKB, I + 2, 'B') && M(SKB, I + 3, 'S') && \
     M(SKB, I + 4, 'Y') && M(SKB, I + 5, 'S') && M(SKB, I + 6, 'T') && M(SKB, I + 7, 'E') && \
     M(SKB, I + 8, 'M') && M(SKB, I + 9, '='))

#define MATCH_0SUBSYSTEM_LENGTH 11
#define MATCH_0SUBSYSTEM(SKB, I)                                                             \
    (M(SKB, I + 0, 0) && M(SKB, I + 1, 'S') && M(SKB, I + 2, 'U') && M(SKB, I + 3, 'B') &&   \
     M(SKB, I + 4, 'S') && M(SKB, I + 5, 'Y') && M(SKB, I + 6, 'S') && M(SKB, I + 7, 'T') && \
     M(SKB, I + 8, 'E') && M(SKB, I + 9, 'M') && M(SKB, I + 10, '='))

SEC("skfilter")
int filter_netlink(struct __sk_buff* skb) {
    __u32 end;
    __u32 i;

    if (skb->protocol != NETLINK_KOBJECT_UEVENT) return TC_ACT_SHOT;
    if (skb->len < NLMSG_HDRLEN) return TC_ACT_SHOT;
    end = load_word(skb, offsetof(struct nlmsghdr, nlmsg_len));
    if (end > skb->len) return TC_ACT_SHOT;
    i = NLMSG_HDRLEN;

    if (i + MATCH_SUBSYSTEM_LENGTH + MATCH_POWER_SUPPLY0_LENGTH > end) return TC_ACT_SHOT;
    if (MATCH_SUBSYSTEM(skb, i)) return MATCH_POWER_SUPPLY0(skb, i + MATCH_SUBSYSTEM_LENGTH);

#pragma unroll
    for (int n = 0; n < 70; ++n) {
        ++i;
        if (i + MATCH_0SUBSYSTEM_LENGTH + MATCH_POWER_SUPPLY0_LENGTH > end) return TC_ACT_SHOT;
        if (MATCH_0SUBSYSTEM(skb, i)) return MATCH_POWER_SUPPLY0(skb, i + MATCH_0SUBSYSTEM_LENGTH);
    }

    return TC_ACT_SHOT;
}

char _license[] SEC("license") = "Apache 2.0";
