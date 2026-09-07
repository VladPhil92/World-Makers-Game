# Child Commerce Compliance Boundaries

This document records product architecture constraints informed by current platform and child-privacy requirements. It is not legal advice and does not replace jurisdiction-specific review before launch.

## World Makers rule set

World Makers intentionally adopts stricter product rules than some platforms require:

- no third-party advertising in the child gameplay experience;
- no rewarded ads;
- no direct purchase flow in the child runtime;
- no purchasable premium currency;
- no paid random rewards;
- parent-owned checkout and family entitlement model;
- child data must not be monetized through behavioral advertising.

## Apple Kids Category boundary

Apple's App Review Guidelines for the Kids Category require purchase opportunities and external links to be reserved for a designated area behind a parental gate. Kids Category apps are also expected to avoid third-party analytics and third-party advertising except in limited compliant circumstances.

Product interpretation for World Makers: all paid commerce is parent-facing; the child runtime contains no direct checkout surface or third-party advertising SDK.

Official source: https://developer.apple.com/app-store/review/guidelines/

## Google Play Families boundary

Google Play Families policies apply to monetization, ads, in-app purchase offers, cross-promotions and other commercial content in apps targeting children. The policy prohibits overly aggressive commercial tactics, emotionally manipulative monetization, deceptive presentation and failure to distinguish virtual currency from real money.

Product interpretation for World Makers: do not use advertising in child gameplay, do not sell premium currency, and do not use urgency/FOMO or pressure-based purchase design.

Official source: https://support.google.com/googleplay/android-developer/answer/9893335

## COPPA / child privacy boundary

The FTC's 2025 COPPA Rule update strengthened restrictions around collection/use/disclosure of children's personal information, including separate parental opt-in for certain third-party disclosures related to targeted advertising and limits on retaining data longer than reasonably necessary.

Product interpretation for World Makers: keep child gameplay identity/pseudonymous progress isolated from commerce/account PII, do not expose child data to advertising ecosystems, and define retention/deletion purposes before production data collection.

Official source: https://www.ftc.gov/news-events/news/press-releases/2025/01/ftc-finalizes-changes-childrens-privacy-rule-limiting-companies-ability-monetize-kids-data

## Colombia and other markets

World Makers must also undergo local legal review for Colombia and every target market, including privacy, consumer protection, digital goods/subscriptions, tax/invoicing, minors' consent, payment processing and platform-specific requirements.

Do not treat this file as a complete launch-compliance checklist.

## Engineering release gate

Production commerce cannot be marked launch-ready until:

1. provider-specific purchase/restore/refund behavior is implemented server-side/parent-side;
2. parent authorization is tested against bypass attempts;
3. entitlement restore/revoke/refund tests exist;
4. privacy/legal review signs off target markets;
5. child UX review confirms there is no coercive purchase pressure;
6. telemetry demonstrates no child PII enters commerce/advertising providers outside an approved legal basis and consent flow.
