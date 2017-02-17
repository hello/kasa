/**
 * system/src/flashdb/slcnand/db.c
 *
 * History:
 *    2007/10/03 - [Charles Chiou] created file
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <flash/nanddb.h>

#define DECLARE_NAND_DB_DEV(x) \
	extern const struct nand_db_s x

/*
 *Note:
 *If two nand flashes have the same ID[1-4] value,
 *Please make sure the ecc-1 bit nand db is ahead of
 *the other ecc-x bit nand db.
*/
DECLARE_NAND_DB_DEV(hy27uf081g2a);
DECLARE_NAND_DB_DEV(hy27uf082g2a);
DECLARE_NAND_DB_DEV(hy27uf082g2b);
DECLARE_NAND_DB_DEV(hy27uf084g2b);
DECLARE_NAND_DB_DEV(hy27uf084g2m);
DECLARE_NAND_DB_DEV(hy27us08121m);
DECLARE_NAND_DB_DEV(hy27us08121a);
DECLARE_NAND_DB_DEV(hy27us08281a);
DECLARE_NAND_DB_DEV(hy27us08561a);
DECLARE_NAND_DB_DEV(hy27us08561m);
DECLARE_NAND_DB_DEV(hy27ua081g1m);
DECLARE_NAND_DB_DEV(hy27u1g8f2b);
DECLARE_NAND_DB_DEV(h27u518s2c);
DECLARE_NAND_DB_DEV(h27u2g8f2c);
DECLARE_NAND_DB_DEV(h27u2g8f2d);
DECLARE_NAND_DB_DEV(k9f1208);
DECLARE_NAND_DB_DEV(k9f1208x0c);
DECLARE_NAND_DB_DEV(k9f1g08);
DECLARE_NAND_DB_DEV(k9f1g08u0b);
DECLARE_NAND_DB_DEV(k9f1g08u0e);
DECLARE_NAND_DB_DEV(k9f4g08u0e);
DECLARE_NAND_DB_DEV(k9f2808);
DECLARE_NAND_DB_DEV(k9f2g08);
DECLARE_NAND_DB_DEV(k9f2g08u0c);
DECLARE_NAND_DB_DEV(k9f5608);
DECLARE_NAND_DB_DEV(k9k4g08);
DECLARE_NAND_DB_DEV(k9k8g08);
DECLARE_NAND_DB_DEV(k9nbg08);
DECLARE_NAND_DB_DEV(k9w8g08);
DECLARE_NAND_DB_DEV(k9wag08);
DECLARE_NAND_DB_DEV(st01gw3a);
DECLARE_NAND_DB_DEV(st01gw3b);
DECLARE_NAND_DB_DEV(st02gw3b);
DECLARE_NAND_DB_DEV(st128w3a);
DECLARE_NAND_DB_DEV(st256w3a);
DECLARE_NAND_DB_DEV(st512w3a);
DECLARE_NAND_DB_DEV(tc58bvg0s3h);
DECLARE_NAND_DB_DEV(tc58bvg1s3h);
DECLARE_NAND_DB_DEV(tc58dvm72a);
DECLARE_NAND_DB_DEV(tc58dvm82a);
DECLARE_NAND_DB_DEV(tc58dvm92a);
DECLARE_NAND_DB_DEV(tc58nvg0s3c);
DECLARE_NAND_DB_DEV(tc58nvg0s3e);
DECLARE_NAND_DB_DEV(tc58nvg0s3h);
DECLARE_NAND_DB_DEV(tc58nvg1s3h);
DECLARE_NAND_DB_DEV(tc58nvg1s3e);
DECLARE_NAND_DB_DEV(tc58nvg2s3e);
DECLARE_NAND_DB_DEV(tc58nvm9s3c);
DECLARE_NAND_DB_DEV(mt29f2g08aac);
DECLARE_NAND_DB_DEV(mt29f1g08abaea);
DECLARE_NAND_DB_DEV(mt29f2g08aba);
DECLARE_NAND_DB_DEV(mt29f8g08daa);
DECLARE_NAND_DB_DEV(mt29f4g08abada);
DECLARE_NAND_DB_DEV(mt29f4g08abbda);
DECLARE_NAND_DB_DEV(mt29f2g08abafa);
DECLARE_NAND_DB_DEV(mt29f2g08abbea);
DECLARE_NAND_DB_DEV(numonyx02gw3b2d);
DECLARE_NAND_DB_DEV(ct48248ns486g1);
DECLARE_NAND_DB_DEV(asu1ga30ht);
DECLARE_NAND_DB_DEV(k9f4g08u0a);
DECLARE_NAND_DB_DEV(f59l1g81a);
DECLARE_NAND_DB_DEV(f59l1g81la);
DECLARE_NAND_DB_DEV(f59l1g81ma);
DECLARE_NAND_DB_DEV(f59l2g81a);
DECLARE_NAND_DB_DEV(f59l4g81a);
DECLARE_NAND_DB_DEV(s34ml01g1);
DECLARE_NAND_DB_DEV(s34ml02g1);
DECLARE_NAND_DB_DEV(s34ml04g1);
DECLARE_NAND_DB_DEV(s34ml01g2);
DECLARE_NAND_DB_DEV(s34ml02g2);
DECLARE_NAND_DB_DEV(s34ml04g2);
DECLARE_NAND_DB_DEV(mx30lf1g08aa);
DECLARE_NAND_DB_DEV(mx30lf1ge8ab);
DECLARE_NAND_DB_DEV(mx30lf2ge8ab);
DECLARE_NAND_DB_DEV(w29n01gvscaa);
DECLARE_NAND_DB_DEV(w29n02gvsiaa);

const struct nand_db_s *G_nand_db[] = {
	&hy27uf081g2a,
	&hy27uf082g2b,
	&hy27uf082g2b,
	&hy27uf084g2b,
	&hy27uf084g2m,
	&hy27us08121m,
	&hy27us08121a,
	&hy27us08281a,
	&hy27us08561a,
	&hy27us08561m,
	&hy27ua081g1m,
	&hy27u1g8f2b,
	&h27u518s2c,
	&h27u2g8f2c,
	&h27u2g8f2d,
	&k9f1208,
	&k9f1208x0c,
	&k9f1g08,
	&k9f1g08u0b,
	&k9f1g08u0e,
	&k9f4g08u0e,
	&k9f2808,
	&k9f2g08,
	&k9f2g08u0c,
	&k9f5608,
	&k9k4g08,
	&k9k8g08,
	&k9nbg08,
	&k9w8g08,
	&k9wag08,
	&st01gw3a,
	&st01gw3b,
	&st02gw3b,
	&st128w3a,
	&st256w3a,
	&st512w3a,
	&tc58bvg0s3h,
	&tc58bvg1s3h,
	&tc58dvm72a,
	&tc58dvm82a,
	&tc58dvm92a,
	&tc58nvg0s3c,
	&tc58nvg0s3e,
	&tc58nvg0s3h,
	&tc58nvg1s3h,
	&tc58nvg1s3e,
	&tc58nvg2s3e,
	&tc58nvm9s3c,
	&mt29f2g08aac,
	&mt29f1g08abaea,
	&mt29f2g08aba,
	&mt29f8g08daa,
	&mt29f4g08abada,
	&mt29f4g08abbda,
	&mt29f2g08abafa,
	&mt29f2g08abbea,
	&numonyx02gw3b2d,
	&ct48248ns486g1,
	&asu1ga30ht,
	&k9f4g08u0a,
	&f59l1g81a,
	&f59l1g81la,
	&f59l1g81ma,
	&f59l2g81a,
	&f59l4g81a,
	&s34ml01g1,
	&s34ml02g1,
	&s34ml04g1,
	&s34ml01g2,
	&s34ml02g2,
	&s34ml04g2,
	&mx30lf1g08aa,
	&mx30lf1ge8ab,
	&mx30lf2ge8ab,
	&w29n01gvscaa,
	&w29n02gvsiaa,
	NULL,
};
