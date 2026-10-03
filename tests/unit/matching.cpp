#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>

#include <QTest>
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "backend/auto_wlr_randr/pattern_matcher.hpp"
#include "backend/auto_wlr_randr/types.hpp"
#include "backend/kanshi/pattern_matcher.hpp"
#include "backend/kanshi/types.hpp"

class MatchingTests : public QObject {
    Q_OBJECT
private slots:

    void assignment_data() {
        QTest::addColumn<bool>("kanshi");
        QTest::newRow("kanshi") << true;
        QTest::newRow("auto-wlr-randr") << false;
    }

    void assignment() {
        QFETCH(bool, kanshi);
        const std::vector<std::vector<std::string>> selectors = {
            { "DP-*", "DP-1", "*" },
            { "DP-1", "DP-1", "*" },
            { "DP-*", "DP-1", "HDMI-2" },
            { "Acme Panel serial-2", "DP-1", "HDMI-*" }
        };
        const std::vector<bool> expected = { true, false, false, true };
        for (size_t row = 0; row < selectors.size(); ++row) {
            std::vector<int> order = { 0, 1, 2 };
            do {
                const std::vector<std::string> names = { "DP-1", "DP-2", "HDMI-1" };
                if (kanshi) {
                    KanshiProfile profile;
                    std::vector<KanshiConnectedOutputInfo> outputs;
                    for (const auto & selector : selectors[row]) {
                        KanshiOutputSetting setting;
                        setting.criteria = selector;
                        profile.outputs.push_back(setting);
                    }
                    outputs.reserve(order.size());
                    for (const auto index : order) {
                        outputs.push_back(
                            { names[index], "Acme", "Panel", "serial-" + std::to_string(index + 1) }
                        );
                    }
                    QCOMPARE(
                        KanshiPatternMatcher::would_profile_match(profile, outputs), expected[row]
                    );
                    outputs.pop_back();
                    QVERIFY(!KanshiPatternMatcher::would_profile_match(profile, outputs));
                } else {
                    AutoWlrRandrProfile profile;
                    std::vector<ConnectedOutputInfo> outputs;
                    for (const auto & selector : selectors[row]) {
                        ProfileOutputSetting setting;
                        setting.output = selector;
                        profile.settings.push_back(setting);
                    }
                    outputs.reserve(order.size());
                    for (const auto index : order) {
                        outputs.push_back(
                            { names[index], "Acme", "Panel", "serial-" + std::to_string(index + 1) }
                        );
                    }
                    QCOMPARE(
                        AutoWlrRandrPatternMatcher::would_profile_match(profile, outputs),
                        expected[row]
                    );
                    outputs.pop_back();
                    QVERIFY(!AutoWlrRandrPatternMatcher::would_profile_match(profile, outputs));
                }
            } while (std::next_permutation(order.begin(), order.end()));
        }
    }
};
QTEST_GUILESS_MAIN(MatchingTests)
#include "matching.moc"
