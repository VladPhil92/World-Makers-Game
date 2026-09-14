#if WITH_DEV_AUTOMATION_TESTS

#include "Launch/WMLaunchBootstrapSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMNativeLaunchTicketParsesTest,
    "WorldMakers.Launch.Bootstrap.TicketUriParses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMNativeLaunchTicketParsesTest::RunTest(const FString& Parameters)
{
    const FString Ticket(TEXT("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"));
    const FString CommandLine = FString::Printf(
        TEXT("WorldMakers.exe -log \"worldmakers://launch?protocol=worldmakers-launch-v2&ticket=%s\""),
        *Ticket);
    FString Parsed;
    TestTrue(TEXT("V2 ticket URI is detected"), UWMLaunchBootstrapSubsystem::TryExtractTicketFromCommandLine(CommandLine, Parsed));
    TestEqual(TEXT("Opaque ticket round-trips"), Parsed, Ticket);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMNativeLaunchTicketRejectsPayloadTest,
    "WorldMakers.Launch.Bootstrap.RejectsPayloadAndLegacy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMNativeLaunchTicketRejectsPayloadTest::RunTest(const FString& Parameters)
{
    const FString Ticket(TEXT("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"));
    FString Parsed;

    TestFalse(
        TEXT("V2 URI carrying a payload is rejected"),
        UWMLaunchBootstrapSubsystem::TryExtractTicketFromCommandLine(
            FString::Printf(TEXT("worldmakers://launch?protocol=worldmakers-launch-v2&ticket=%s&payload=forbidden"), *Ticket),
            Parsed));
    TestFalse(
        TEXT("Legacy v1 payload URI is not accepted by v2 bootstrap"),
        UWMLaunchBootstrapSubsystem::TryExtractTicketFromCommandLine(
            TEXT("worldmakers://launch?protocol=worldmakers-launch-v1&payload=abc"),
            Parsed));
    TestFalse(
        TEXT("Short ticket is rejected"),
        UWMLaunchBootstrapSubsystem::TryExtractTicketFromCommandLine(
            TEXT("worldmakers://launch?protocol=worldmakers-launch-v2&ticket=short"),
            Parsed));
    TestFalse(
        TEXT("Non-base64url ticket characters are rejected"),
        UWMLaunchBootstrapSubsystem::TryExtractTicketFromCommandLine(
            TEXT("worldmakers://launch?protocol=worldmakers-launch-v2&ticket=abcdefghijklmnopqrstuvwxyzABCDE!GHIJKLMNOPQRSTUVWXYZ0123456789_-"),
            Parsed));
    return true;
}

#endif
