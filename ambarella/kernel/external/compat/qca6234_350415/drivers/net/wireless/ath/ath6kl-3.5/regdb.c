/*
 * Copyright (c) 2004-2013 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <linux/nl80211.h>
#include <net/cfg80211.h>

/*
 * Another solution is to use CONFIG_CFG80211_INTERNAL_REGDB to merge ath6kl's
 * regdb into cfg80211.ko.
 *
 * Please copy this file to cfg80211 module path and turn on
 * CONFIG_ATH6KL_REGDB_AS_CFG80211_REGDB then rebuilt cfg80211.ko.
 */

#ifdef CONFIG_ATH6KL_REGDB_AS_CFG80211_REGDB
#undef CONFIG_ATH6KL_REGDB_AS_CFG80211_REGDB
#endif

#ifdef CONFIG_ATH6KL_REGDB_AS_CFG80211_REGDB
#include "regdb.h"
#endif

/*
 * Regulatory data: Not exactly sync to the target's because the host
 *                  just need to know the channel-list, bandwidth, DFS,
 *                  IBSS and Passive. Others (like powers, rates...) are
 *                  handled by the target.
 *
 * WARN: Generated from program and please not to modify it.
 */
static const struct ieee80211_regdomain ath6kl_regd_AL = {
	.n_reg_rules = 4,
	.alpha2 = "AL",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_DZ = {
	.n_reg_rules = 4,
	.alpha2 = "DZ",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_AR = {
	.n_reg_rules = 5,
	.alpha2 = "AR",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_AM = {
	.n_reg_rules = 3,
	.alpha2 = "AM",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 18, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_AW = {
	.n_reg_rules = 4,
	.alpha2 = "AW",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_AU = {
	.n_reg_rules = 5,
	.alpha2 = "AU",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_A2 = {
	.n_reg_rules = 6,
	.alpha2 = "A2",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5580 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5660 - 10, 5700 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_AT = {
	.n_reg_rules = 4,
	.alpha2 = "AT",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_AZ = {
	.n_reg_rules = 3,
	.alpha2 = "AZ",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 18, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_BH = {
	.n_reg_rules = 4,
	.alpha2 = "BH",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_BD = {
	.n_reg_rules = 1,
	.alpha2 = "BD",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_BB = {
	.n_reg_rules = 4,
	.alpha2 = "BB",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_BY = {
	.n_reg_rules = 4,
	.alpha2 = "BY",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_BE = {
	.n_reg_rules = 4,
	.alpha2 = "BE",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_BZ = {
	.n_reg_rules = 2,
	.alpha2 = "BZ",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 30, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_BO = {
	.n_reg_rules = 2,
	.alpha2 = "BO",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 30, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_BA = {
	.n_reg_rules = 4,
	.alpha2 = "BA",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_BR = {
	.n_reg_rules = 5,
	.alpha2 = "BR",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_BN = {
	.n_reg_rules = 4,
	.alpha2 = "BN",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_BG = {
	.n_reg_rules = 4,
	.alpha2 = "BG",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_KH = {
	.n_reg_rules = 4,
	.alpha2 = "KH",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_CA = {
	.n_reg_rules = 5,
	.alpha2 = "CA",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_C2 = {
	.n_reg_rules = 6,
	.alpha2 = "C2",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5580 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5660 - 10, 5700 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_CL = {
	.n_reg_rules = 4,
	.alpha2 = "CL",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_CN = {
	.n_reg_rules = 2,
	.alpha2 = "CN",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_CO = {
	.n_reg_rules = 5,
	.alpha2 = "CO",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_CR = {
	.n_reg_rules = 4,
	.alpha2 = "CR",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_HR = {
	.n_reg_rules = 4,
	.alpha2 = "HR",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_CY = {
	.n_reg_rules = 3,
	.alpha2 = "CY",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_CZ = {
	.n_reg_rules = 4,
	.alpha2 = "CZ",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_DK = {
	.n_reg_rules = 4,
	.alpha2 = "DK",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_DO = {
	.n_reg_rules = 4,
	.alpha2 = "DO",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_EC = {
	.n_reg_rules = 4,
	.alpha2 = "EC",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_EG = {
	.n_reg_rules = 3,
	.alpha2 = "EG",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_SV = {
	.n_reg_rules = 4,
	.alpha2 = "SV",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_EE = {
	.n_reg_rules = 4,
	.alpha2 = "EE",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_FI = {
	.n_reg_rules = 4,
	.alpha2 = "FI",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_FR = {
	.n_reg_rules = 4,
	.alpha2 = "FR",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_F2 = {
	.n_reg_rules = 3,
	.alpha2 = "F2",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_GE = {
	.n_reg_rules = 3,
	.alpha2 = "GE",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 18, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_DE = {
	.n_reg_rules = 4,
	.alpha2 = "DE",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_GR = {
	.n_reg_rules = 4,
	.alpha2 = "GR",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_GL = {
	.n_reg_rules = 4,
	.alpha2 = "GL",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_GD = {
	.n_reg_rules = 5,
	.alpha2 = "GD",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_GU = {
	.n_reg_rules = 4,
	.alpha2 = "GU",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_GT = {
	.n_reg_rules = 4,
	.alpha2 = "GT",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_HT = {
	.n_reg_rules = 4,
	.alpha2 = "HT",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_HN = {
	.n_reg_rules = 5,
	.alpha2 = "HN",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_HK = {
	.n_reg_rules = 5,
	.alpha2 = "HK",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_HU = {
	.n_reg_rules = 4,
	.alpha2 = "HU",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_IS = {
	.n_reg_rules = 4,
	.alpha2 = "IS",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_IN = {
	.n_reg_rules = 4,
	.alpha2 = "IN",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_ID = {
	.n_reg_rules = 2,
	.alpha2 = "ID",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5745 - 10, 5805 + 10, 20, 3, 23, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_IR = {
	.n_reg_rules = 2,
	.alpha2 = "IR",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_IE = {
	.n_reg_rules = 4,
	.alpha2 = "IE",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_IL = {
	.n_reg_rules = 3,
	.alpha2 = "IL",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_IT = {
	.n_reg_rules = 4,
	.alpha2 = "IT",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_JM = {
	.n_reg_rules = 5,
	.alpha2 = "JM",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_JP = {
	.n_reg_rules = 6,
	.alpha2 = "JP",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 20, 0),
		REG_RULE(2467 - 10, 2472 + 10, 20, 3, 20, 0),
		REG_RULE(2484 - 10, 2484 + 10, 20, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_JO = {
	.n_reg_rules = 3,
	.alpha2 = "JO",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 23, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_KZ = {
	.n_reg_rules = 1,
	.alpha2 = "KZ",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_KE = {
	.n_reg_rules = 4,
	.alpha2 = "KE",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5500 - 10, 5560 + 10, 40, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5765 + 10, 40, 3, 23, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_KP = {
	.n_reg_rules = 5,
	.alpha2 = "KP",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 20, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5620 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5805 + 10, 20, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_KR = {
	.n_reg_rules = 5,
	.alpha2 = "KR",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5805 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_K2 = {
	.n_reg_rules = 3,
	.alpha2 = "K2",
	.reg_rules = {
		REG_RULE(2312 - 10, 2372 + 10, 20, 3, 20, 0),
		REG_RULE(2412 - 10, 2472 + 10, 20, 3, 20, 0),
		REG_RULE(5745 - 10, 5805 + 10, 20, 3, 23, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_K3 = {
	.n_reg_rules = 5,
	.alpha2 = "K3",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5620 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5805 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_KW = {
	.n_reg_rules = 3,
	.alpha2 = "KW",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_LV = {
	.n_reg_rules = 4,
	.alpha2 = "LV",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_LB = {
	.n_reg_rules = 5,
	.alpha2 = "LB",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_LI = {
	.n_reg_rules = 4,
	.alpha2 = "LI",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_LT = {
	.n_reg_rules = 4,
	.alpha2 = "LT",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_LU = {
	.n_reg_rules = 4,
	.alpha2 = "LU",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_MO = {
	.n_reg_rules = 2,
	.alpha2 = "MO",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_MK = {
	.n_reg_rules = 4,
	.alpha2 = "MK",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_MW = {
	.n_reg_rules = 4,
	.alpha2 = "MW",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_MY = {
	.n_reg_rules = 4,
	.alpha2 = "MY",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_MT = {
	.n_reg_rules = 4,
	.alpha2 = "MT",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_MX = {
	.n_reg_rules = 4,
	.alpha2 = "MX",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_MC = {
	.n_reg_rules = 3,
	.alpha2 = "MC",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 18, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_MA = {
	.n_reg_rules = 3,
	.alpha2 = "MA",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 23, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_NP = {
	.n_reg_rules = 4,
	.alpha2 = "NP",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_NZ = {
	.n_reg_rules = 5,
	.alpha2 = "NZ",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_NL = {
	.n_reg_rules = 4,
	.alpha2 = "NL",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_AN = {
	.n_reg_rules = 4,
	.alpha2 = "AN",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_NO = {
	.n_reg_rules = 4,
	.alpha2 = "NO",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_OM = {
	.n_reg_rules = 5,
	.alpha2 = "OM",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_PK = {
	.n_reg_rules = 2,
	.alpha2 = "PK",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_PA = {
	.n_reg_rules = 4,
	.alpha2 = "PA",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_PE = {
	.n_reg_rules = 5,
	.alpha2 = "PE",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_PH = {
	.n_reg_rules = 5,
	.alpha2 = "PH",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_PL = {
	.n_reg_rules = 4,
	.alpha2 = "PL",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_PT = {
	.n_reg_rules = 4,
	.alpha2 = "PT",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_PR = {
	.n_reg_rules = 5,
	.alpha2 = "PR",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_QA = {
	.n_reg_rules = 2,
	.alpha2 = "QA",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_RO = {
	.n_reg_rules = 4,
	.alpha2 = "RO",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_RU = {
	.n_reg_rules = 5,
	.alpha2 = "RU",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5660 - 10, 5700 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_RW = {
	.n_reg_rules = 5,
	.alpha2 = "RW",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_SA = {
	.n_reg_rules = 5,
	.alpha2 = "SA",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_ME = {
	.n_reg_rules = 4,
	.alpha2 = "ME",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_RS = {
	.n_reg_rules = 4,
	.alpha2 = "RS",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_SG = {
	.n_reg_rules = 5,
	.alpha2 = "SG",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_SK = {
	.n_reg_rules = 4,
	.alpha2 = "SK",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_SI = {
	.n_reg_rules = 4,
	.alpha2 = "SI",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_ZA = {
	.n_reg_rules = 5,
	.alpha2 = "ZA",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_ES = {
	.n_reg_rules = 4,
	.alpha2 = "ES",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_LK = {
	.n_reg_rules = 5,
	.alpha2 = "LK",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 20, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_SE = {
	.n_reg_rules = 4,
	.alpha2 = "SE",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_CH = {
	.n_reg_rules = 4,
	.alpha2 = "CH",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_SY = {
	.n_reg_rules = 1,
	.alpha2 = "SY",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_TW = {
	.n_reg_rules = 5,
	.alpha2 = "TW",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5280 - 10, 5320 + 10, 40, 3, 17, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5580 + 10, 40, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5660 - 10, 5700 + 10, 40, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_TH = {
	.n_reg_rules = 5,
	.alpha2 = "TH",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_TT = {
	.n_reg_rules = 5,
	.alpha2 = "TT",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_TN = {
	.n_reg_rules = 3,
	.alpha2 = "TN",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_TR = {
	.n_reg_rules = 4,
	.alpha2 = "TR",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_UG = {
	.n_reg_rules = 5,
	.alpha2 = "UG",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5805 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_UA = {
	.n_reg_rules = 5,
	.alpha2 = "UA",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5660 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5745 - 10, 5805 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_AE = {
	.n_reg_rules = 5,
	.alpha2 = "AE",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_GB = {
	.n_reg_rules = 4,
	.alpha2 = "GB",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_US = {
	.n_reg_rules = 5,
	.alpha2 = "US",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_U2 = {
	.n_reg_rules = 6,
	.alpha2 = "U2",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5580 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5660 - 10, 5700 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_PS = {
	.n_reg_rules = 4,
	.alpha2 = "PS",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 23, 0
						 | NL80211_RRF_DFS),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_UY = {
	.n_reg_rules = 5,
	.alpha2 = "UY",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_UZ = {
	.n_reg_rules = 5,
	.alpha2 = "UZ",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_VE = {
	.n_reg_rules = 4,
	.alpha2 = "VE",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_VN = {
	.n_reg_rules = 5,
	.alpha2 = "VN",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_YE = {
	.n_reg_rules = 1,
	.alpha2 = "YE",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_ZW = {
	.n_reg_rules = 4,
	.alpha2 = "ZW",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_00 = {
	.n_reg_rules = 6,
	.alpha2 = "00",
	.reg_rules = {
		REG_RULE(2312 - 10, 2372 + 10, 20, 3, 5, 0),
		REG_RULE(2412 - 10, 2472 + 10, 20, 3, 5, 0),
		REG_RULE(2484 - 10, 2484 + 10, 20, 3, 5, 0),
		REG_RULE(5120 - 10, 5240 + 10, 20, 3, 5, 0),
		REG_RULE(5260 - 10, 5700 + 10, 20, 3, 5, 0
						 | NL80211_RRF_DFS),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 5, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_03 = {
	.n_reg_rules = 1,
	.alpha2 = "03",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_07 = {
	.n_reg_rules = 1,
	.alpha2 = "07",
	.reg_rules = {
		REG_RULE(2432 - 10, 2442 + 10, 20, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_08 = {
	.n_reg_rules = 1,
	.alpha2 = "08",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_20 = {
	.n_reg_rules = 4,
	.alpha2 = "20",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_21 = {
	.n_reg_rules = 4,
	.alpha2 = "21",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_22 = {
	.n_reg_rules = 4,
	.alpha2 = "22",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_3a = {
	.n_reg_rules = 5,
	.alpha2 = "3a",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_3b = {
	.n_reg_rules = 5,
	.alpha2 = "3b",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_3f = {
	.n_reg_rules = 5,
	.alpha2 = "3f",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5720 + 10, 40, 3, 24, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_12 = {
	.n_reg_rules = 4,
	.alpha2 = "12",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 23, 0
						 | NL80211_RRF_DFS),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_13 = {
	.n_reg_rules = 2,
	.alpha2 = "13",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_16 = {
	.n_reg_rules = 2,
	.alpha2 = "16",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_14 = {
	.n_reg_rules = 6,
	.alpha2 = "14",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5580 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5660 - 10, 5700 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_23 = {
	.n_reg_rules = 6,
	.alpha2 = "23",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5580 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5660 - 10, 5700 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_37 = {
	.n_reg_rules = 4,
	.alpha2 = "37",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 27, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_35 = {
	.n_reg_rules = 2,
	.alpha2 = "35",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 18, 0
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_36 = {
	.n_reg_rules = 3,
	.alpha2 = "36",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_30 = {
	.n_reg_rules = 3,
	.alpha2 = "30",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 18, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_39 = {
	.n_reg_rules = 2,
	.alpha2 = "39",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 15, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_34 = {
	.n_reg_rules = 4,
	.alpha2 = "34",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5280 + 10, 20, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_3d = {
	.n_reg_rules = 5,
	.alpha2 = "3d",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5660 - 10, 5700 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_3e = {
	.n_reg_rules = 5,
	.alpha2 = "3e",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5660 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5745 - 10, 5805 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_31 = {
	.n_reg_rules = 3,
	.alpha2 = "31",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_11 = {
	.n_reg_rules = 4,
	.alpha2 = "11",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_10 = {
	.n_reg_rules = 4,
	.alpha2 = "10",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 17, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 23, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_52 = {
	.n_reg_rules = 2,
	.alpha2 = "52",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_45 = {
	.n_reg_rules = 2,
	.alpha2 = "45",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5745 - 10, 5805 + 10, 40, 3, 23, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_4d = {
	.n_reg_rules = 2,
	.alpha2 = "4d",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5745 - 10, 5805 + 10, 40, 3, 23, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_47 = {
	.n_reg_rules = 3,
	.alpha2 = "47",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5280 - 10, 5320 + 10, 40, 3, 17, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 17, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_50 = {
	.n_reg_rules = 3,
	.alpha2 = "50",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5280 - 10, 5320 + 10, 40, 3, 17, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 17, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_42 = {
	.n_reg_rules = 3,
	.alpha2 = "42",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 23, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_58 = {
	.n_reg_rules = 2,
	.alpha2 = "58",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 17, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_5b = {
	.n_reg_rules = 4,
	.alpha2 = "5b",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 20, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_5c = {
	.n_reg_rules = 5,
	.alpha2 = "5c",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5280 - 10, 5320 + 10, 40, 3, 17, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5580 + 10, 40, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5660 - 10, 5700 + 10, 40, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_5d = {
	.n_reg_rules = 3,
	.alpha2 = "5d",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_5e = {
	.n_reg_rules = 5,
	.alpha2 = "5e",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5620 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5805 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_5f = {
	.n_reg_rules = 5,
	.alpha2 = "5f",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5805 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_51 = {
	.n_reg_rules = 4,
	.alpha2 = "51",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 23, 0),
		REG_RULE(5500 - 10, 5560 + 10, 40, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5765 + 10, 40, 3, 23, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_55 = {
	.n_reg_rules = 2,
	.alpha2 = "55",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 30, 0),
		REG_RULE(5745 - 10, 5825 + 10, 40, 3, 30, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_56 = {
	.n_reg_rules = 2,
	.alpha2 = "56",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 30, 0),
		REG_RULE(5745 - 10, 5805 + 10, 40, 3, 23, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_49 = {
	.n_reg_rules = 3,
	.alpha2 = "49",
	.reg_rules = {
		REG_RULE(2312 - 10, 2372 + 10, 20, 3, 20, 0),
		REG_RULE(2412 - 10, 2472 + 10, 20, 3, 20, 0),
		REG_RULE(5745 - 10, 5805 + 10, 40, 3, 23, 0),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_99 = {
	.n_reg_rules = 6,
	.alpha2 = "99",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 20, 0),
		REG_RULE(2467 - 10, 2472 + 10, 20, 3, 20, 0),
		REG_RULE(2484 - 10, 2484 + 10, 20, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_9a = {
	.n_reg_rules = 4,
	.alpha2 = "9a",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_88 = {
	.n_reg_rules = 4,
	.alpha2 = "88",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_d4 = {
	.n_reg_rules = 8,
	.alpha2 = "d4",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 20, 0),
		REG_RULE(2467 - 10, 2472 + 10, 20, 3, 20, 0),
		REG_RULE(2484 - 10, 2484 + 10, 20, 3, 20, 0),
		REG_RULE(4920 - 10, 4980 + 10, 20, 3, 23, 0),
		REG_RULE(5040 - 10, 5080 + 10, 20, 3, 23, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_d5 = {
	.n_reg_rules = 6,
	.alpha2 = "d5",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 30, 0),
		REG_RULE(4920 - 10, 4980 + 10, 20, 3, 23, 0),
		REG_RULE(5040 - 10, 5080 + 10, 20, 3, 23, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_d7 = {
	.n_reg_rules = 6,
	.alpha2 = "d7",
	.reg_rules = {
		REG_RULE(2412 - 10, 2472 + 10, 40, 3, 20, 0),
		REG_RULE(4920 - 10, 4980 + 10, 20, 3, 23, 0),
		REG_RULE(5040 - 10, 5080 + 10, 20, 3, 23, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_87 = {
	.n_reg_rules = 6,
	.alpha2 = "87",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 40, 3, 20, 0),
		REG_RULE(2467 - 10, 2472 + 10, 20, 3, 20, 0),
		REG_RULE(2484 - 10, 2484 + 10, 20, 3, 20, 0),
		REG_RULE(5180 - 10, 5240 + 10, 40, 3, 20, 0),
		REG_RULE(5260 - 10, 5320 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 40, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_60 = {
	.n_reg_rules = 13,
	.alpha2 = "60",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(2467 - 10, 2467 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(2472 - 10, 2472 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(2484 - 10, 2484 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5170 - 10, 5230 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_61 = {
	.n_reg_rules = 13,
	.alpha2 = "61",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(2467 - 10, 2467 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(2472 - 10, 2472 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(2484 - 10, 2484 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5170 - 10, 5230 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_62 = {
	.n_reg_rules = 13,
	.alpha2 = "62",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(2467 - 10, 2467 + 10, 20, 3, 18, 0),
		REG_RULE(2472 - 10, 2472 + 10, 20, 3, 18, 0),
		REG_RULE(2484 - 10, 2484 + 10, 20, 3, 18, 0),
		REG_RULE(5170 - 10, 5230 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_63 = {
	.n_reg_rules = 11,
	.alpha2 = "63",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(2467 - 10, 2467 + 10, 20, 3, 18, 0),
		REG_RULE(2472 - 10, 2472 + 10, 20, 3, 18, 0),
		REG_RULE(5170 - 10, 5230 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_64 = {
	.n_reg_rules = 8,
	.alpha2 = "64",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_65 = {
	.n_reg_rules = 10,
	.alpha2 = "65",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(2467 - 10, 2467 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(2472 - 10, 2472 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_66 = {
	.n_reg_rules = 10,
	.alpha2 = "66",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(5170 - 10, 5230 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_67 = {
	.n_reg_rules = 12,
	.alpha2 = "67",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(2467 - 10, 2467 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(2472 - 10, 2472 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5170 - 10, 5230 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_68 = {
	.n_reg_rules = 12,
	.alpha2 = "68",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(2467 - 10, 2467 + 10, 20, 3, 18, 0),
		REG_RULE(2472 - 10, 2472 + 10, 20, 3, 18, 0),
		REG_RULE(5170 - 10, 5230 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_69 = {
	.n_reg_rules = 9,
	.alpha2 = "69",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_6a = {
	.n_reg_rules = 11,
	.alpha2 = "6a",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(2467 - 10, 2467 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(2472 - 10, 2472 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_6b = {
	.n_reg_rules = 10,
	.alpha2 = "6b",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(2467 - 10, 2467 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(2472 - 10, 2472 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

static const struct ieee80211_regdomain ath6kl_regd_6c = {
	.n_reg_rules = 11,
	.alpha2 = "6c",
	.reg_rules = {
		REG_RULE(2412 - 10, 2412 + 10, 20, 3, 18, 0),
		REG_RULE(2417 - 10, 2432 + 10, 20, 3, 18, 0),
		REG_RULE(2437 - 10, 2442 + 10, 20, 3, 18, 0),
		REG_RULE(2447 - 10, 2457 + 10, 20, 3, 18, 0),
		REG_RULE(2462 - 10, 2462 + 10, 20, 3, 18, 0),
		REG_RULE(2467 - 10, 2467 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(2472 - 10, 2472 + 10, 20, 3, 18, 0
						 | NL80211_RRF_PASSIVE_SCAN),
		REG_RULE(5180 - 10, 5240 + 10, 20, 3, 30, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5260 - 10, 5320 + 10, 20, 3, 18, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5745 - 10, 5825 + 10, 20, 3, 20, 0
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
		REG_RULE(5500 - 10, 5700 + 10, 20, 3, 20, 0
						 | NL80211_RRF_DFS
						 | NL80211_RRF_PASSIVE_SCAN
						 | NL80211_RRF_NO_IBSS),
	}
};

#ifndef CONFIG_ATH6KL_REGDB_AS_CFG80211_REGDB
const struct ieee80211_regdomain *ath6kl_reg_regdb_country[] = {
	&ath6kl_regd_AL,
	&ath6kl_regd_DZ,
	&ath6kl_regd_AR,
	&ath6kl_regd_AM,
	&ath6kl_regd_AW,
	&ath6kl_regd_AU,
	&ath6kl_regd_A2,
	&ath6kl_regd_AT,
	&ath6kl_regd_AZ,
	&ath6kl_regd_BH,
	&ath6kl_regd_BD,
	&ath6kl_regd_BB,
	&ath6kl_regd_BY,
	&ath6kl_regd_BE,
	&ath6kl_regd_BZ,
	&ath6kl_regd_BO,
	&ath6kl_regd_BA,
	&ath6kl_regd_BR,
	&ath6kl_regd_BN,
	&ath6kl_regd_BG,
	&ath6kl_regd_KH,
	&ath6kl_regd_CA,
	&ath6kl_regd_C2,
	&ath6kl_regd_CL,
	&ath6kl_regd_CN,
	&ath6kl_regd_CO,
	&ath6kl_regd_CR,
	&ath6kl_regd_HR,
	&ath6kl_regd_CY,
	&ath6kl_regd_CZ,
	&ath6kl_regd_DK,
	&ath6kl_regd_DO,
	&ath6kl_regd_EC,
	&ath6kl_regd_EG,
	&ath6kl_regd_SV,
	&ath6kl_regd_EE,
	&ath6kl_regd_FI,
	&ath6kl_regd_FR,
	&ath6kl_regd_F2,
	&ath6kl_regd_GE,
	&ath6kl_regd_DE,
	&ath6kl_regd_GR,
	&ath6kl_regd_GL,
	&ath6kl_regd_GD,
	&ath6kl_regd_GU,
	&ath6kl_regd_GT,
	&ath6kl_regd_HT,
	&ath6kl_regd_HN,
	&ath6kl_regd_HK,
	&ath6kl_regd_HU,
	&ath6kl_regd_IS,
	&ath6kl_regd_IN,
	&ath6kl_regd_ID,
	&ath6kl_regd_IR,
	&ath6kl_regd_IE,
	&ath6kl_regd_IL,
	&ath6kl_regd_IT,
	&ath6kl_regd_JM,
	&ath6kl_regd_JP,
	&ath6kl_regd_JO,
	&ath6kl_regd_KZ,
	&ath6kl_regd_KE,
	&ath6kl_regd_KP,
	&ath6kl_regd_KR,
	&ath6kl_regd_K2,
	&ath6kl_regd_K3,
	&ath6kl_regd_KW,
	&ath6kl_regd_LV,
	&ath6kl_regd_LB,
	&ath6kl_regd_LI,
	&ath6kl_regd_LT,
	&ath6kl_regd_LU,
	&ath6kl_regd_MO,
	&ath6kl_regd_MK,
	&ath6kl_regd_MW,
	&ath6kl_regd_MY,
	&ath6kl_regd_MT,
	&ath6kl_regd_MX,
	&ath6kl_regd_MC,
	&ath6kl_regd_MA,
	&ath6kl_regd_NP,
	&ath6kl_regd_NZ,
	&ath6kl_regd_NL,
	&ath6kl_regd_AN,
	&ath6kl_regd_NO,
	&ath6kl_regd_OM,
	&ath6kl_regd_PK,
	&ath6kl_regd_PA,
	&ath6kl_regd_PE,
	&ath6kl_regd_PH,
	&ath6kl_regd_PL,
	&ath6kl_regd_PT,
	&ath6kl_regd_PR,
	&ath6kl_regd_QA,
	&ath6kl_regd_RO,
	&ath6kl_regd_RU,
	&ath6kl_regd_RW,
	&ath6kl_regd_SA,
	&ath6kl_regd_ME,
	&ath6kl_regd_RS,
	&ath6kl_regd_SG,
	&ath6kl_regd_SK,
	&ath6kl_regd_SI,
	&ath6kl_regd_ZA,
	&ath6kl_regd_ES,
	&ath6kl_regd_LK,
	&ath6kl_regd_SE,
	&ath6kl_regd_CH,
	&ath6kl_regd_SY,
	&ath6kl_regd_TW,
	&ath6kl_regd_TH,
	&ath6kl_regd_TT,
	&ath6kl_regd_TN,
	&ath6kl_regd_TR,
	&ath6kl_regd_UG,
	&ath6kl_regd_UA,
	&ath6kl_regd_AE,
	&ath6kl_regd_GB,
	&ath6kl_regd_US,
	&ath6kl_regd_U2,
	&ath6kl_regd_PS,
	&ath6kl_regd_UY,
	&ath6kl_regd_UZ,
	&ath6kl_regd_VE,
	&ath6kl_regd_VN,
	&ath6kl_regd_YE,
	&ath6kl_regd_ZW,
	NULL,		/* keep last */
};

const struct ieee80211_regdomain *ath6kl_reg_regdb_region[] = {
	&ath6kl_regd_00,
	&ath6kl_regd_03,
	&ath6kl_regd_07,
	&ath6kl_regd_08,
	&ath6kl_regd_20,
	&ath6kl_regd_21,
	&ath6kl_regd_22,
	&ath6kl_regd_3a,
	&ath6kl_regd_3b,
	&ath6kl_regd_3f,
	&ath6kl_regd_12,
	&ath6kl_regd_13,
	&ath6kl_regd_16,
	&ath6kl_regd_14,
	&ath6kl_regd_23,
	&ath6kl_regd_37,
	&ath6kl_regd_35,
	&ath6kl_regd_36,
	&ath6kl_regd_30,
	&ath6kl_regd_39,
	&ath6kl_regd_34,
	&ath6kl_regd_3d,
	&ath6kl_regd_3e,
	&ath6kl_regd_31,
	&ath6kl_regd_11,
	&ath6kl_regd_10,
	&ath6kl_regd_52,
	&ath6kl_regd_45,
	&ath6kl_regd_4d,
	&ath6kl_regd_47,
	&ath6kl_regd_50,
	&ath6kl_regd_42,
	&ath6kl_regd_58,
	&ath6kl_regd_5b,
	&ath6kl_regd_5c,
	&ath6kl_regd_5d,
	&ath6kl_regd_5e,
	&ath6kl_regd_5f,
	&ath6kl_regd_51,
	&ath6kl_regd_55,
	&ath6kl_regd_56,
	&ath6kl_regd_49,
	&ath6kl_regd_99,
	&ath6kl_regd_9a,
	&ath6kl_regd_88,
	&ath6kl_regd_d4,
	&ath6kl_regd_d5,
	&ath6kl_regd_d7,
	&ath6kl_regd_87,
	&ath6kl_regd_60,
	&ath6kl_regd_61,
	&ath6kl_regd_62,
	&ath6kl_regd_63,
	&ath6kl_regd_64,
	&ath6kl_regd_65,
	&ath6kl_regd_66,
	&ath6kl_regd_67,
	&ath6kl_regd_68,
	&ath6kl_regd_69,
	&ath6kl_regd_6a,
	&ath6kl_regd_6b,
	&ath6kl_regd_6c,
	NULL,		/* keep last */
};
#else
const struct ieee80211_regdomain *reg_regdb[] = {
	&ath6kl_regd_AL,
	&ath6kl_regd_DZ,
	&ath6kl_regd_AR,
	&ath6kl_regd_AM,
	&ath6kl_regd_AW,
	&ath6kl_regd_AU,
	&ath6kl_regd_A2,
	&ath6kl_regd_AT,
	&ath6kl_regd_AZ,
	&ath6kl_regd_BH,
	&ath6kl_regd_BD,
	&ath6kl_regd_BB,
	&ath6kl_regd_BY,
	&ath6kl_regd_BE,
	&ath6kl_regd_BZ,
	&ath6kl_regd_BO,
	&ath6kl_regd_BA,
	&ath6kl_regd_BR,
	&ath6kl_regd_BN,
	&ath6kl_regd_BG,
	&ath6kl_regd_KH,
	&ath6kl_regd_CA,
	&ath6kl_regd_C2,
	&ath6kl_regd_CL,
	&ath6kl_regd_CN,
	&ath6kl_regd_CO,
	&ath6kl_regd_CR,
	&ath6kl_regd_HR,
	&ath6kl_regd_CY,
	&ath6kl_regd_CZ,
	&ath6kl_regd_DK,
	&ath6kl_regd_DO,
	&ath6kl_regd_EC,
	&ath6kl_regd_EG,
	&ath6kl_regd_SV,
	&ath6kl_regd_EE,
	&ath6kl_regd_FI,
	&ath6kl_regd_FR,
	&ath6kl_regd_F2,
	&ath6kl_regd_GE,
	&ath6kl_regd_DE,
	&ath6kl_regd_GR,
	&ath6kl_regd_GL,
	&ath6kl_regd_GD,
	&ath6kl_regd_GU,
	&ath6kl_regd_GT,
	&ath6kl_regd_HT,
	&ath6kl_regd_HN,
	&ath6kl_regd_HK,
	&ath6kl_regd_HU,
	&ath6kl_regd_IS,
	&ath6kl_regd_IN,
	&ath6kl_regd_ID,
	&ath6kl_regd_IR,
	&ath6kl_regd_IE,
	&ath6kl_regd_IL,
	&ath6kl_regd_IT,
	&ath6kl_regd_JM,
	&ath6kl_regd_JP,
	&ath6kl_regd_JO,
	&ath6kl_regd_KZ,
	&ath6kl_regd_KE,
	&ath6kl_regd_KP,
	&ath6kl_regd_KR,
	&ath6kl_regd_K2,
	&ath6kl_regd_K3,
	&ath6kl_regd_KW,
	&ath6kl_regd_LV,
	&ath6kl_regd_LB,
	&ath6kl_regd_LI,
	&ath6kl_regd_LT,
	&ath6kl_regd_LU,
	&ath6kl_regd_MO,
	&ath6kl_regd_MK,
	&ath6kl_regd_MW,
	&ath6kl_regd_MY,
	&ath6kl_regd_MT,
	&ath6kl_regd_MX,
	&ath6kl_regd_MC,
	&ath6kl_regd_MA,
	&ath6kl_regd_NP,
	&ath6kl_regd_NZ,
	&ath6kl_regd_NL,
	&ath6kl_regd_AN,
	&ath6kl_regd_NO,
	&ath6kl_regd_OM,
	&ath6kl_regd_PK,
	&ath6kl_regd_PA,
	&ath6kl_regd_PE,
	&ath6kl_regd_PH,
	&ath6kl_regd_PL,
	&ath6kl_regd_PT,
	&ath6kl_regd_PR,
	&ath6kl_regd_QA,
	&ath6kl_regd_RO,
	&ath6kl_regd_RU,
	&ath6kl_regd_RW,
	&ath6kl_regd_SA,
	&ath6kl_regd_ME,
	&ath6kl_regd_RS,
	&ath6kl_regd_SG,
	&ath6kl_regd_SK,
	&ath6kl_regd_SI,
	&ath6kl_regd_ZA,
	&ath6kl_regd_ES,
	&ath6kl_regd_LK,
	&ath6kl_regd_SE,
	&ath6kl_regd_CH,
	&ath6kl_regd_SY,
	&ath6kl_regd_TW,
	&ath6kl_regd_TH,
	&ath6kl_regd_TT,
	&ath6kl_regd_TN,
	&ath6kl_regd_TR,
	&ath6kl_regd_UG,
	&ath6kl_regd_UA,
	&ath6kl_regd_AE,
	&ath6kl_regd_GB,
	&ath6kl_regd_US,
	&ath6kl_regd_U2,
	&ath6kl_regd_PS,
	&ath6kl_regd_UY,
	&ath6kl_regd_UZ,
	&ath6kl_regd_VE,
	&ath6kl_regd_VN,
	&ath6kl_regd_YE,
	&ath6kl_regd_ZW,
	&ath6kl_regd_00,
	&ath6kl_regd_03,
	&ath6kl_regd_07,
	&ath6kl_regd_08,
	&ath6kl_regd_20,
	&ath6kl_regd_21,
	&ath6kl_regd_22,
	&ath6kl_regd_3a,
	&ath6kl_regd_3b,
	&ath6kl_regd_3f,
	&ath6kl_regd_12,
	&ath6kl_regd_13,
	&ath6kl_regd_16,
	&ath6kl_regd_14,
	&ath6kl_regd_23,
	&ath6kl_regd_37,
	&ath6kl_regd_35,
	&ath6kl_regd_36,
	&ath6kl_regd_30,
	&ath6kl_regd_39,
	&ath6kl_regd_34,
	&ath6kl_regd_3d,
	&ath6kl_regd_3e,
	&ath6kl_regd_31,
	&ath6kl_regd_11,
	&ath6kl_regd_10,
	&ath6kl_regd_52,
	&ath6kl_regd_45,
	&ath6kl_regd_4d,
	&ath6kl_regd_47,
	&ath6kl_regd_50,
	&ath6kl_regd_42,
	&ath6kl_regd_58,
	&ath6kl_regd_5b,
	&ath6kl_regd_5c,
	&ath6kl_regd_5d,
	&ath6kl_regd_5e,
	&ath6kl_regd_5f,
	&ath6kl_regd_51,
	&ath6kl_regd_55,
	&ath6kl_regd_56,
	&ath6kl_regd_49,
	&ath6kl_regd_99,
	&ath6kl_regd_9a,
	&ath6kl_regd_88,
	&ath6kl_regd_d4,
	&ath6kl_regd_d5,
	&ath6kl_regd_d7,
	&ath6kl_regd_87,
	&ath6kl_regd_60,
	&ath6kl_regd_61,
	&ath6kl_regd_62,
	&ath6kl_regd_63,
	&ath6kl_regd_64,
	&ath6kl_regd_65,
	&ath6kl_regd_66,
	&ath6kl_regd_67,
	&ath6kl_regd_68,
	&ath6kl_regd_69,
	&ath6kl_regd_6a,
	&ath6kl_regd_6b,
	&ath6kl_regd_6c,
};

int reg_regdb_size = ARRAY_SIZE(reg_regdb);
#endif

