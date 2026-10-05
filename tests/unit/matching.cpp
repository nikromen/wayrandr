#include "backend/profile/matching.hpp"

#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>

#include <QTest>
#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "backend/auto_wlr_randr/profile_backend.hpp"
#include "backend/kanshi/profile_backend.hpp"
#include "backend/profile/types.hpp"

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
        auto invalid_glob = profile::matching::InvalidGlobBehavior::NO_MATCH;
        if (kanshi) {
            invalid_glob = profile::matching::InvalidGlobBehavior::LITERAL_NAME;
        }
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
                profile::ProfileDefinition profile;
                std::vector<profile::ConnectedOutput> outputs;
                for (const auto & selector : selectors[row]) {
                    profile::ProfileOutputDefinition setting;
                    setting.output = selector;
                    profile.outputs.push_back(setting);
                }
                outputs.reserve(order.size());
                for (const auto index : order) {
                    outputs.push_back(
                        { names[index], "Acme", "Panel", "serial-" + std::to_string(index + 1) }
                    );
                }
                QCOMPARE(
                    profile::matching::would_profile_match(profile, outputs, invalid_glob),
                    expected[row]
                );
                outputs.pop_back();
                QVERIFY(!profile::matching::would_profile_match(profile, outputs, invalid_glob));
            } while (std::next_permutation(order.begin(), order.end()));
        }
    }

    void auto_wlr_ordered_assignment() {
        const AutoWlrRandrProfileBackend backend;
        profile::ProfileDefinition profile;
        profile::ProfileOutputDefinition wildcard;
        wildcard.output = "DP-*";
        wildcard.enabled = false;
        profile::ProfileOutputDefinition exact;
        exact.output = "DP-1";
        profile.outputs = { wildcard, exact };
        std::vector<profile::ConnectedOutput> outputs = {
            { "DP-1", std::nullopt, std::nullopt, std::nullopt },
            { "DP-2", std::nullopt, std::nullopt, std::nullopt }
        };

        // auto-wlr-randr 1.2.0 consumes DP-1 for DP-* and cannot reuse it for DP-1.
        QCOMPARE(
            backend.get_match_warning(profile, outputs),
            std::string("Profile patterns do not match the currently connected outputs.")
        );
        std::reverse(outputs.begin(), outputs.end());
        QVERIFY(backend.get_match_warning(profile, outputs).empty());
        std::reverse(outputs.begin(), outputs.end());
        std::reverse(profile.outputs.begin(), profile.outputs.end());
        QVERIFY(backend.get_match_warning(profile, outputs).empty());
    }

    void invalid_glob() {
        const KanshiProfileBackend kanshi;
        const AutoWlrRandrProfileBackend auto_wlr;
        const std::vector<profile::ConnectedOutput> outputs{
            { "DP[1", std::nullopt, std::nullopt, std::nullopt }
        };
        profile::ProfileDefinition profile;
        profile::ProfileOutputDefinition output;
        output.output = "DP[1";
        profile.outputs.push_back(output);
        QCOMPARE(kanshi.describe_output_match("DP[1", outputs), std::string("Matches DP[1"));
        QCOMPARE(
            auto_wlr.describe_output_match("DP[1", outputs),
            std::string("No connected output matches this pattern")
        );
        QVERIFY(kanshi.get_match_warning(profile, outputs).empty());
        QCOMPARE(
            auto_wlr.get_match_warning(profile, outputs),
            std::string("Profile patterns do not match the currently connected outputs.")
        );
        QVERIFY(!profile::matching::find_matching_output(
            "DP[1", outputs, profile::matching::InvalidGlobBehavior::NO_MATCH
        ));
    }
};
QTEST_GUILESS_MAIN(MatchingTests)
#include "matching.moc"
