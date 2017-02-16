/*
 * DO NOT EDIT -- file generated from data in db.txt
 */

#include <linux/nl80211.h>
#include <net/cfg80211.h>
#include "regdb.h"

static const struct ieee80211_regdomain regdom_00 = {
	.alpha2 = "00",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 20, 0),
		REG_RULE(2457, 2482, 20, 3, 20, 
			NL80211_RRF_PASSIVE_SCAN | 
			NL80211_RRF_NO_IBSS | 0),
		REG_RULE(2474, 2494, 20, 3, 20, 
			NL80211_RRF_PASSIVE_SCAN | 
			NL80211_RRF_NO_IBSS | 
			NL80211_RRF_NO_OFDM | 0),
		REG_RULE(5140, 5360, 40, 0, 30, 
			NL80211_RRF_PASSIVE_SCAN | 
			NL80211_RRF_NO_IBSS | 0),
		REG_RULE(5460, 5860, 40, 0, 30, 
			NL80211_RRF_PASSIVE_SCAN | 
			NL80211_RRF_NO_IBSS | 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_AE = {
	.alpha2 = "AE",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_AL = {
	.alpha2 = "AL",
	.reg_rules = {
		REG_RULE(2402, 2482, 20, 0, 20, 0),
	},
	.n_reg_rules = 1
};

static const struct ieee80211_regdomain regdom_AM = {
	.alpha2 = "AM",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 18, 0),
		REG_RULE(5250, 5330, 20, 0, 18, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_AN = {
	.alpha2 = "AN",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_AR = {
	.alpha2 = "AR",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_AT = {
	.alpha2 = "AT",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_AU = {
	.alpha2 = "AU",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 23, 0),
		REG_RULE(5250, 5330, 40, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_AW = {
	.alpha2 = "AW",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_AZ = {
	.alpha2 = "AZ",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 18, 0),
		REG_RULE(5250, 5330, 40, 0, 18, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_BA = {
	.alpha2 = "BA",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BB = {
	.alpha2 = "BB",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 23, 0),
		REG_RULE(5250, 5330, 40, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BD = {
	.alpha2 = "BD",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
	},
	.n_reg_rules = 1
};

static const struct ieee80211_regdomain regdom_BE = {
	.alpha2 = "BE",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BG = {
	.alpha2 = "BG",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 23, 0),
		REG_RULE(5250, 5290, 40, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 30, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BH = {
	.alpha2 = "BH",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 20, 0),
		REG_RULE(5250, 5330, 20, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 0, 20, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BL = {
	.alpha2 = "BL",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 18, 0),
		REG_RULE(5250, 5330, 40, 0, 18, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_BN = {
	.alpha2 = "BN",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BO = {
	.alpha2 = "BO",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 30, 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_BR = {
	.alpha2 = "BR",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_BY = {
	.alpha2 = "BY",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BZ = {
	.alpha2 = "BZ",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 30, 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_CA = {
	.alpha2 = "CA",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_CH = {
	.alpha2 = "CH",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_CL = {
	.alpha2 = "CL",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 0, 20, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_CN = {
	.alpha2 = "CN",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_CO = {
	.alpha2 = "CO",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_CR = {
	.alpha2 = "CR",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 3, 17, 0),
		REG_RULE(5250, 5330, 20, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_CS = {
	.alpha2 = "CS",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_CY = {
	.alpha2 = "CY",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_CZ = {
	.alpha2 = "CZ",
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 40, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 40, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_DE = {
	.alpha2 = "DE",
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 40, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 40, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_DK = {
	.alpha2 = "DK",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_DO = {
	.alpha2 = "DO",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_DZ = {
	.alpha2 = "DZ",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
	},
	.n_reg_rules = 1
};

static const struct ieee80211_regdomain regdom_EC = {
	.alpha2 = "EC",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 3, 17, 0),
		REG_RULE(5250, 5330, 20, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_EE = {
	.alpha2 = "EE",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_EG = {
	.alpha2 = "EG",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 20, 0),
		REG_RULE(5250, 5330, 20, 0, 20, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_ES = {
	.alpha2 = "ES",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_FI = {
	.alpha2 = "FI",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_FR = {
	.alpha2 = "FR",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_GE = {
	.alpha2 = "GE",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 18, 0),
		REG_RULE(5250, 5330, 40, 0, 18, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_GB = {
	.alpha2 = "GB",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_GD = {
	.alpha2 = "GD",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_GR = {
	.alpha2 = "GR",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_GL = {
	.alpha2 = "GL",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 20, 0),
		REG_RULE(5250, 5330, 20, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 20, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_GT = {
	.alpha2 = "GT",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_GU = {
	.alpha2 = "GU",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5170, 5250, 20, 3, 17, 0),
		REG_RULE(5250, 5330, 20, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_HN = {
	.alpha2 = "HN",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_HK = {
	.alpha2 = "HK",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_HR = {
	.alpha2 = "HR",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_HT = {
	.alpha2 = "HT",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_HU = {
	.alpha2 = "HU",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_ID = {
	.alpha2 = "ID",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
	},
	.n_reg_rules = 1
};

static const struct ieee80211_regdomain regdom_IE = {
	.alpha2 = "IE",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_IL = {
	.alpha2 = "IL",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 40, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 40, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_IN = {
	.alpha2 = "IN",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 0, 20, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_IS = {
	.alpha2 = "IS",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_IR = {
	.alpha2 = "IR",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_IT = {
	.alpha2 = "IT",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_JM = {
	.alpha2 = "JM",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_JP = {
	.alpha2 = "JP",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 20, 0),
		REG_RULE(2457, 2482, 20, 0, 20, 0),
		REG_RULE(2474, 2494, 20, 0, 20, 
			NL80211_RRF_NO_OFDM | 0),
		REG_RULE(4910, 4930, 10, 0, 23, 0),
		REG_RULE(4910, 4990, 40, 0, 23, 0),
		REG_RULE(4930, 4950, 10, 0, 23, 0),
		REG_RULE(5030, 5045, 10, 0, 23, 0),
		REG_RULE(5030, 5090, 40, 0, 23, 0),
		REG_RULE(5050, 5060, 10, 0, 23, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 23, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 12
};

static const struct ieee80211_regdomain regdom_JO = {
	.alpha2 = "JO",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 18, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_KE = {
	.alpha2 = "KE",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_KH = {
	.alpha2 = "KH",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_KP = {
	.alpha2 = "KP",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5330, 40, 3, 20, 0),
		REG_RULE(5160, 5250, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5630, 40, 3, 30, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5815, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_KR = {
	.alpha2 = "KR",
	.reg_rules = {
		REG_RULE(2402, 2482, 20, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 3, 20, 0),
		REG_RULE(5250, 5330, 20, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5630, 20, 3, 30, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5815, 20, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_KW = {
	.alpha2 = "KW",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_KZ = {
	.alpha2 = "KZ",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
	},
	.n_reg_rules = 1
};

static const struct ieee80211_regdomain regdom_LB = {
	.alpha2 = "LB",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_LI = {
	.alpha2 = "LI",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_LK = {
	.alpha2 = "LK",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 3, 17, 0),
		REG_RULE(5250, 5330, 20, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 20, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_LT = {
	.alpha2 = "LT",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_LU = {
	.alpha2 = "LU",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_LV = {
	.alpha2 = "LV",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_MC = {
	.alpha2 = "MC",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 18, 0),
		REG_RULE(5250, 5330, 40, 0, 18, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_MA = {
	.alpha2 = "MA",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
	},
	.n_reg_rules = 1
};

static const struct ieee80211_regdomain regdom_MO = {
	.alpha2 = "MO",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 23, 0),
		REG_RULE(5250, 5330, 40, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_MK = {
	.alpha2 = "MK",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_MT = {
	.alpha2 = "MT",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_MY = {
	.alpha2 = "MY",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 30, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_MX = {
	.alpha2 = "MX",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_NL = {
	.alpha2 = "NL",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_NO = {
	.alpha2 = "NO",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_NP = {
	.alpha2 = "NP",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_NZ = {
	.alpha2 = "NZ",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 20, 3, 23, 0),
		REG_RULE(5250, 5330, 20, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_OM = {
	.alpha2 = "OM",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_PA = {
	.alpha2 = "PA",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_PE = {
	.alpha2 = "PE",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_PG = {
	.alpha2 = "PG",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_PH = {
	.alpha2 = "PH",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_PK = {
	.alpha2 = "PK",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_PL = {
	.alpha2 = "PL",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_PT = {
	.alpha2 = "PT",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_PR = {
	.alpha2 = "PR",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_QA = {
	.alpha2 = "QA",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_RO = {
	.alpha2 = "RO",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_RU = {
	.alpha2 = "RU",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 20, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_SA = {
	.alpha2 = "SA",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 3, 23, 0),
		REG_RULE(5250, 5330, 20, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_SE = {
	.alpha2 = "SE",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_SG = {
	.alpha2 = "SG",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 0, 20, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_SI = {
	.alpha2 = "SI",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_SK = {
	.alpha2 = "SK",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_SV = {
	.alpha2 = "SV",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 3, 17, 0),
		REG_RULE(5250, 5330, 20, 3, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 3, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_SY = {
	.alpha2 = "SY",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
	},
	.n_reg_rules = 1
};

static const struct ieee80211_regdomain regdom_TW = {
	.alpha2 = "TW",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5270, 5330, 40, 3, 17, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5815, 40, 3, 30, 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_TH = {
	.alpha2 = "TH",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_TT = {
	.alpha2 = "TT",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_TN = {
	.alpha2 = "TN",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 20, 0),
		REG_RULE(5250, 5330, 20, 0, 20, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_TR = {
	.alpha2 = "TR",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 20, 0),
		REG_RULE(5250, 5330, 20, 0, 20, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_UA = {
	.alpha2 = "UA",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
	},
	.n_reg_rules = 1
};

static const struct ieee80211_regdomain regdom_US = {
	.alpha2 = "US",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5600, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5650, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_UY = {
	.alpha2 = "UY",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_UZ = {
	.alpha2 = "UZ",
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 3, 27, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_VE = {
	.alpha2 = "VE",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5815, 40, 0, 23, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_VN = {
	.alpha2 = "VN",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_YE = {
	.alpha2 = "YE",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
	},
	.n_reg_rules = 1
};

static const struct ieee80211_regdomain regdom_ZA = {
	.alpha2 = "ZA",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 3, 17, 0),
		REG_RULE(5250, 5330, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 3, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 3, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_ZW = {
	.alpha2 = "ZW",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
	},
	.n_reg_rules = 1
};

const struct ieee80211_regdomain *reg_regdb[] = {
	&regdom_00,
	&regdom_AE,
	&regdom_AL,
	&regdom_AM,
	&regdom_AN,
	&regdom_AR,
	&regdom_AT,
	&regdom_AU,
	&regdom_AW,
	&regdom_AZ,
	&regdom_BA,
	&regdom_BB,
	&regdom_BD,
	&regdom_BE,
	&regdom_BG,
	&regdom_BH,
	&regdom_BL,
	&regdom_BN,
	&regdom_BO,
	&regdom_BR,
	&regdom_BY,
	&regdom_BZ,
	&regdom_CA,
	&regdom_CH,
	&regdom_CL,
	&regdom_CN,
	&regdom_CO,
	&regdom_CR,
	&regdom_CS,
	&regdom_CY,
	&regdom_CZ,
	&regdom_DE,
	&regdom_DK,
	&regdom_DO,
	&regdom_DZ,
	&regdom_EC,
	&regdom_EE,
	&regdom_EG,
	&regdom_ES,
	&regdom_FI,
	&regdom_FR,
	&regdom_GE,
	&regdom_GB,
	&regdom_GD,
	&regdom_GR,
	&regdom_GL,
	&regdom_GT,
	&regdom_GU,
	&regdom_HN,
	&regdom_HK,
	&regdom_HR,
	&regdom_HT,
	&regdom_HU,
	&regdom_ID,
	&regdom_IE,
	&regdom_IL,
	&regdom_IN,
	&regdom_IS,
	&regdom_IR,
	&regdom_IT,
	&regdom_JM,
	&regdom_JP,
	&regdom_JO,
	&regdom_KE,
	&regdom_KH,
	&regdom_KP,
	&regdom_KR,
	&regdom_KW,
	&regdom_KZ,
	&regdom_LB,
	&regdom_LI,
	&regdom_LK,
	&regdom_LT,
	&regdom_LU,
	&regdom_LV,
	&regdom_MC,
	&regdom_MA,
	&regdom_MO,
	&regdom_MK,
	&regdom_MT,
	&regdom_MY,
	&regdom_MX,
	&regdom_NL,
	&regdom_NO,
	&regdom_NP,
	&regdom_NZ,
	&regdom_OM,
	&regdom_PA,
	&regdom_PE,
	&regdom_PG,
	&regdom_PH,
	&regdom_PK,
	&regdom_PL,
	&regdom_PT,
	&regdom_PR,
	&regdom_QA,
	&regdom_RO,
	&regdom_RU,
	&regdom_SA,
	&regdom_SE,
	&regdom_SG,
	&regdom_SI,
	&regdom_SK,
	&regdom_SV,
	&regdom_SY,
	&regdom_TW,
	&regdom_TH,
	&regdom_TT,
	&regdom_TN,
	&regdom_TR,
	&regdom_UA,
	&regdom_US,
	&regdom_UY,
	&regdom_UZ,
	&regdom_VE,
	&regdom_VN,
	&regdom_YE,
	&regdom_ZA,
	&regdom_ZW,
};

int reg_regdb_size = ARRAY_SIZE(reg_regdb);
