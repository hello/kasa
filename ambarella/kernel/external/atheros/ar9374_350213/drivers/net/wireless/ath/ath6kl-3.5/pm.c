/*
 * Copyright (c) 2004-2011 Atheros Communications Inc.
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

#include "core.h"
#include "hif-ops.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ath6kl_early_suspend(struct early_suspend *handler)
{
	struct ath6kl *ar = container_of(handler, struct ath6kl, early_suspend);

	if (ar)
		ath6kl_hif_early_suspend(ar);
}

static void ath6kl_late_resume(struct early_suspend *handler)
{
	struct ath6kl *ar = container_of(handler, struct ath6kl, early_suspend);

	if (ar)
		ath6kl_hif_late_resume(ar);
}
#endif

#ifdef CONFIG_ANDROID
void ath6kl_setup_android_resource(struct ath6kl *ar)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	ar->early_suspend.suspend = ath6kl_early_suspend;
	ar->early_suspend.resume = ath6kl_late_resume;
	ar->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	register_early_suspend(&ar->early_suspend);
#endif
}

void ath6kl_cleanup_android_resource(struct ath6kl *ar)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ar->early_suspend);
#endif
}

#endif
