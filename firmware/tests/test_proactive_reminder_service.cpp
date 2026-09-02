#include <assert.h>
#include <stdio.h>

#include "reminder/ProactiveReminderService.h"

using namespace tongdou;

namespace {

constexpr uint32_t kSecond = 1000;
constexpr uint32_t kMinute = 60 * kSecond;

ProactiveReminderSettings settings(uint16_t activityMinutes,
                                   uint16_t hydrationMinutes) {
  ProactiveReminderSettings value;
  value.activityIntervalMinutes = activityMinutes;
  value.hydrationIntervalMinutes = hydrationMinutes;
  return value;
}

void testHydrationOnlyRepeatsAtConfiguredInterval() {
  ProactiveReminderSettings value = settings(1440, 1);
  value.activityEnabled = false;
  ProactiveReminderService service(value);
  service.begin(0);
  service.update(kSecond, true);
  service.update(61 * kSecond, true);
  assert(service.takeDue(61 * kSecond, true) == ProactiveReminderType::Hydration);
  assert(service.takeDue(61 * kSecond, true) == ProactiveReminderType::None);
  service.delivered(ProactiveReminderType::Hydration, 61 * kSecond);
  service.update(120 * kSecond, true);
  assert(service.takeDue(120 * kSecond, true) == ProactiveReminderType::None);
  service.update(121 * kSecond, true);
  assert(service.takeDue(121 * kSecond, true) == ProactiveReminderType::Hydration);
}

void testActivityOnlyRepeatsAtConfiguredInterval() {
  ProactiveReminderSettings value = settings(1, 1440);
  value.hydrationEnabled = false;
  ProactiveReminderService service(value);
  service.begin(0);
  service.update(kSecond, true);
  service.update(61 * kSecond, true);
  assert(service.takeDue(61 * kSecond, true) == ProactiveReminderType::ActivityBreak);
  service.delivered(ProactiveReminderType::ActivityBreak, 61 * kSecond);
  service.update(120 * kSecond, true);
  assert(service.takeDue(120 * kSecond, true) == ProactiveReminderType::None);
  service.update(121 * kSecond, true);
  assert(service.takeDue(121 * kSecond, true) == ProactiveReminderType::ActivityBreak);
}

void testShortAbsenceDoesNotReset() {
  ProactiveReminderService service(settings(2, 1440));
  service.begin(0);
  service.update(kSecond, true);
  service.update(61 * kSecond, false);
  service.update(120 * kSecond, false);
  service.update(121 * kSecond, true);
  assert(service.takeDue(121 * kSecond, true) == ProactiveReminderType::ActivityBreak);
}

void testLongAbsenceResetsBothTimers() {
  ProactiveReminderService service(settings(1, 1));
  service.begin(0);
  service.update(kSecond, true);
  service.update(61 * kSecond, false);
  service.update(361 * kSecond, false);
  service.update(362 * kSecond, true);
  service.update(421 * kSecond, true);
  assert(service.takeDue(421 * kSecond, true) == ProactiveReminderType::None);
  service.update(422 * kSecond, true);
  assert(service.takeDue(422 * kSecond, true) == ProactiveReminderType::ActivityBreak);
  service.delivered(ProactiveReminderType::ActivityBreak, 422 * kSecond);
  service.update(722 * kSecond, true);
  assert(service.takeDue(722 * kSecond, true) == ProactiveReminderType::Hydration);
}

void testDisabledAndChangedIntervals() {
  ProactiveReminderSettings value = settings(2, 1440);
  value.activityEnabled = false;
  ProactiveReminderService service(value);
  service.begin(0);
  service.update(kSecond, true);
  service.update(300 * kSecond, true);
  assert(service.takeDue(300 * kSecond, true) == ProactiveReminderType::None);

  value.activityEnabled = true;
  value.activityIntervalMinutes = 1;
  service.setSettings(value);
  service.update(301 * kSecond, true);
  assert(service.takeDue(301 * kSecond, true) == ProactiveReminderType::ActivityBreak);
}

void testCrossTypeCooldownKeepsSecondPending() {
  ProactiveReminderService service(settings(1, 1));
  service.begin(0);
  service.update(kSecond, true);
  service.update(61 * kSecond, true);
  assert(service.takeDue(61 * kSecond, false) == ProactiveReminderType::None);
  assert(service.takeDue(61 * kSecond, true) == ProactiveReminderType::ActivityBreak);
  service.delivered(ProactiveReminderType::ActivityBreak, 61 * kSecond);
  service.update(121 * kSecond, true);
  assert(service.takeDue(121 * kSecond, true) == ProactiveReminderType::None);
  service.update(360 * kSecond, true);
  assert(service.takeDue(360 * kSecond, true) == ProactiveReminderType::None);
  service.update(361 * kSecond, true);
  assert(service.takeDue(361 * kSecond, true) == ProactiveReminderType::Hydration);
}

void testSimultaneousEventsAlternateAcrossRounds() {
  ProactiveReminderService service(settings(1, 1));
  service.begin(0);
  service.update(kSecond, true);

  service.update(61 * kSecond, true);
  assert(service.takeDue(61 * kSecond, true) == ProactiveReminderType::ActivityBreak);
  service.delivered(ProactiveReminderType::ActivityBreak, 61 * kSecond);

  service.update(361 * kSecond, true);
  assert(service.takeDue(361 * kSecond, true) == ProactiveReminderType::Hydration);
  service.delivered(ProactiveReminderType::Hydration, 361 * kSecond);

  service.update(661 * kSecond, true);
  assert(service.takeDue(661 * kSecond, true) == ProactiveReminderType::ActivityBreak);
  service.delivered(ProactiveReminderType::ActivityBreak, 661 * kSecond);

  service.update(961 * kSecond, true);
  assert(service.takeDue(961 * kSecond, true) == ProactiveReminderType::Hydration);
}

void testLongIntervalDoesNotRegress() {
  ProactiveReminderSettings value = settings(6, 1440);
  value.hydrationEnabled = false;
  ProactiveReminderService service(value);
  service.begin(0);
  service.update(kSecond, true);
  service.update(361 * kSecond, true);
  assert(service.takeDue(361 * kSecond, true) == ProactiveReminderType::ActivityBreak);
  service.delivered(ProactiveReminderType::ActivityBreak, 361 * kSecond);
  service.update(720 * kSecond, true);
  assert(service.takeDue(720 * kSecond, true) == ProactiveReminderType::None);
  service.update(721 * kSecond, true);
  assert(service.takeDue(721 * kSecond, true) == ProactiveReminderType::ActivityBreak);
}

}  // namespace

int main() {
#define RUN_TEST(test)   \
  printf("%s...", #test); \
  fflush(stdout);       \
  test();               \
  puts("PASS")

  RUN_TEST(testHydrationOnlyRepeatsAtConfiguredInterval);
  RUN_TEST(testActivityOnlyRepeatsAtConfiguredInterval);
  RUN_TEST(testShortAbsenceDoesNotReset);
  RUN_TEST(testLongAbsenceResetsBothTimers);
  RUN_TEST(testDisabledAndChangedIntervals);
  RUN_TEST(testCrossTypeCooldownKeepsSecondPending);
  RUN_TEST(testSimultaneousEventsAlternateAcrossRounds);
  RUN_TEST(testLongIntervalDoesNotRegress);
#undef RUN_TEST
  return 0;
}
