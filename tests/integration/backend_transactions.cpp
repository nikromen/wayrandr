#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>

#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#include <QMetaObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <clocale>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "backend.hpp"
#include "backend/base.hpp"
#include "backend/wlr_randr.hpp"
#include "monitor_specs.hpp"
#include "support/environment.hpp"
#include "support/fixture.hpp"
#include "utils/helpers.hpp"

class BackendTransactions : public QObject {
    Q_OBJECT
private slots:

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.

    void confirmation_and_fresh_snapshot() {
        auto fixture = std::make_shared<Fixture>();
        JobDrain drain;
        QVERIFY2(std::setlocale(LC_NUMERIC, "cs_CZ.UTF-8") != nullptr, "Czech locale unavailable");
        auto report = std::make_shared<BackendReport>();
        auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [fixture, report, result] {
                auto & f = *fixture;

                auto target = f.target();
                BackendManager manager;
                manager.apply_with_confirmation(target, nullptr);
                report->verify(
                    manager.has_pending_changes() && manager.can_confirm(),
                    "Apply must await confirmation"
                );
                const auto calls = f.read("calls");
                report->throws([&] { manager.apply(target); });
                report->throws([&] { manager.apply_with_confirmation(target, nullptr); });
                for (auto type :
                     { BackendType::WLR_RANDR, BackendType::KANSHI, BackendType::AUTO_WLR_RANDR }) {
                    report->throws([&] { manager.set_backend(type); });
                }
                report->verify(f.read("calls") == calls, "Blocked operations must not invoke CLI");
                manager.cancel_apply();
                report->verify(
                    !manager.has_pending_changes() && f.state()[0]["scale"] == 1,
                    "Cancel restores original snapshot"
                );
                manager.cancel_apply();
                manager.confirm_apply();
                manager.apply_with_confirmation(target, nullptr);
                manager.confirm_apply();
                manager.cancel_apply();
                report->verify(
                    !manager.has_pending_changes() && f.state()[0]["scale"] == 2,
                    "Confirm keeps applied state"
                );
                manager.apply_with_confirmation(target, nullptr);
                manager.cancel_apply();
                report->verify(
                    f.state()[0]["scale"] == 2, "Next snapshot must reflect real current state"
                );


                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 20000);
        for (const auto & observation : report->observations) {
            QVERIFY2(observation.passed, observation.message.c_str());
        }
        QVERIFY2(result->error.empty(), result->error.c_str());
    }

    void external_snapshot() {
        auto fixture = std::make_shared<Fixture>();
        JobDrain drain;
        QVERIFY2(std::setlocale(LC_NUMERIC, "cs_CZ.UTF-8") != nullptr, "Czech locale unavailable");
        auto report = std::make_shared<BackendReport>();
        auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [fixture, report, result] {
                auto & f = *fixture;

                auto target = f.target();
                BackendManager manager;
                f.reset();
                auto external = f.state();
                external[0]["scale"] = 3;
                f.write("state", external.dump());
                manager.apply(target);
                manager.cancel_apply();
                report->verify(
                    f.state()[0]["scale"] == 3, "Snapshot must precede Apply and use actual state"
                );

                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 20000);
        for (const auto & observation : report->observations) {
            QVERIFY2(observation.passed, observation.message.c_str());
        }
        QVERIFY2(result->error.empty(), result->error.c_str());
    }

    void partial_apply_failure() {
        auto fixture = std::make_shared<Fixture>();
        JobDrain drain;
        QVERIFY2(std::setlocale(LC_NUMERIC, "cs_CZ.UTF-8") != nullptr, "Czech locale unavailable");
        auto report = std::make_shared<BackendReport>();
        auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [fixture, report, result] {
                auto & f = *fixture;

                auto target = f.target();
                BackendManager manager;
                f.reset();
                f.write("failures", "1");
                report->throws([&] { manager.apply_with_confirmation(target, nullptr); });
                report->verify(
                    !manager.has_pending_changes() && f.state()[0]["scale"] == 1,
                    "Failed Apply must recover partial changes"
                );
                f.write("failures", "2");
                report->throws([&] { manager.apply_with_confirmation(target, nullptr); });
                report->verify(
                    manager.has_pending_changes() && !manager.can_confirm(),
                    "Double failure retains recovery state"
                );
                manager.confirm_apply();
                report->verify(manager.has_pending_changes(), "Cannot confirm a failed Apply");
                report->throws([&] { manager.set_backend(BackendType::AUTO_WLR_RANDR); });
                manager.cancel_apply();
                report->verify(
                    !manager.has_pending_changes() && f.state()[0]["scale"] == 1,
                    "Retry retains original snapshot"
                );


                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 20000);
        for (const auto & observation : report->observations) {
            QVERIFY2(observation.passed, observation.message.c_str());
        }
        QVERIFY2(result->error.empty(), result->error.c_str());
    }

    void rollback_retry() {
        auto fixture = std::make_shared<Fixture>();
        JobDrain drain;
        QVERIFY2(std::setlocale(LC_NUMERIC, "cs_CZ.UTF-8") != nullptr, "Czech locale unavailable");
        auto report = std::make_shared<BackendReport>();
        auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [fixture, report, result] {
                auto & f = *fixture;

                auto target = f.target();
                BackendManager manager;
                manager.apply_with_confirmation(target, nullptr);
                f.write("failures", "1");
                report->throws([&] { manager.cancel_apply(); });
                report->verify(
                    manager.has_pending_changes() && !manager.can_confirm(),
                    "Failed rollback must not be confirmable"
                );
                manager.cancel_apply();
                report->verify(
                    !manager.has_pending_changes(), "Successful retry resolves rollback"
                );


                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 20000);
        for (const auto & observation : report->observations) {
            QVERIFY2(observation.passed, observation.message.c_str());
        }
        QVERIFY2(result->error.empty(), result->error.c_str());
    }

    void invalid_snapshots() {
        auto fixture = std::make_shared<Fixture>();
        JobDrain drain;
        QVERIFY2(std::setlocale(LC_NUMERIC, "cs_CZ.UTF-8") != nullptr, "Czech locale unavailable");
        auto report = std::make_shared<BackendReport>();
        auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [fixture, report, result] {
                auto & f = *fixture;

                auto target = f.target();
                BackendManager manager;
                target[0].set_enabled(false);
                const auto before = f.read("calls");
                report->throws([&] { manager.apply(target); });
                report->verify(
                    f.read("calls") == before && !manager.has_pending_changes(),
                    "All-off must be rejected before CLI"
                );
                target[0].set_enabled(true);
                for (const auto & bad :
                     { std::string("[]"), std::string("invalid json"), std::string("[{}]") }) {
                    f.write("state", bad);
                    report->throws([&] { manager.apply(target); });
                    report->verify(
                        !manager.has_pending_changes(), "Invalid snapshot must not change state"
                    );
                }
                f.reset();
                auto partial = f.state();
                partial.push_back(json::object());
                f.write("state", partial.dump());
                report->throws([&] { manager.apply(target); });
                report->verify(!manager.has_pending_changes(), "Partial snapshot must be rejected");
                f.reset();
                auto state = f.state();
                state[0]["modes"][0]["current"] = false;
                f.write("state", state.dump());
                report->throws([&] { manager.apply(target); });
                f.reset();
                f.write("query_failure", "1");
                report->throws([&] { manager.apply(target); });
                report->verify(
                    !manager.has_pending_changes(), "Snapshot query failure must prevent Apply"
                );
                f.reset();


                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 20000);
        for (const auto & observation : report->observations) {
            QVERIFY2(observation.passed, observation.message.c_str());
        }
        QVERIFY2(result->error.empty(), result->error.c_str());
    }

    void disconnected_outputs() {
        auto fixture = std::make_shared<Fixture>();
        JobDrain drain;
        QVERIFY2(std::setlocale(LC_NUMERIC, "cs_CZ.UTF-8") != nullptr, "Czech locale unavailable");
        auto report = std::make_shared<BackendReport>();
        auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [fixture, report, result] {
                auto & f = *fixture;

                auto target = f.target();
                BackendManager manager;
                // One disappearing output must not prevent restoration of survivors.
                auto state = f.state();
                auto second = state[0];
                second["name"] = "DP-2";
                state.push_back(second);
                f.write("state", state.dump());
                target = f.target();
                manager.apply(target);
                state = f.state();
                state.erase(1);
                f.write("state", state.dump());
                manager.cancel_apply();
                report->verify(
                    !manager.has_pending_changes() && f.state()[0]["scale"] == 1,
                    "Restore connected outputs only"
                );
                target = f.target();
                manager.apply(target);
                f.write("state", "[]");
                report->throws([&] { manager.cancel_apply(); });
                report->verify(
                    manager.has_pending_changes(), "No surviving output must retain snapshot"
                );
                f.reset();
                manager.cancel_apply();
                report->verify(!manager.has_pending_changes(), "Reconnect permits manual retry");


                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 20000);
        for (const auto & observation : report->observations) {
            QVERIFY2(observation.passed, observation.message.c_str());
        }
        QVERIFY2(result->error.empty(), result->error.c_str());
    }

    void disabled_survivor() {
        auto fixture = std::make_shared<Fixture>();
        JobDrain drain;
        QVERIFY2(std::setlocale(LC_NUMERIC, "cs_CZ.UTF-8") != nullptr, "Czech locale unavailable");
        auto report = std::make_shared<BackendReport>();
        auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [fixture, report, result] {
                auto & f = *fixture;

                auto target = f.target();
                BackendManager manager;
                // A surviving originally disabled output must not be switched off when the
                // only originally enabled output has disappeared.
                auto state = f.state();
                auto second = state[0];
                second["name"] = "DP-2";
                state[0]["enabled"] = false;
                state.push_back(second);
                f.write("state", state.dump());
                auto reconnect_state = state;
                target = get_monitor_specs_list();
                target[0].set_enabled(true);
                manager.apply(target);
                state = f.state();
                state.erase(1);
                f.write("state", state.dump());
                const auto before_unsafe_rollback = f.read("calls");
                report->throws([&] { manager.cancel_apply(); });
                report->verify(
                    manager.has_pending_changes() && f.state()[0]["enabled"] == true,
                    "Rollback must not disable the only surviving output"
                );
                report->verify(
                    f.read("calls") == before_unsafe_rollback + "[\"--json\"]\n",
                    "Unsafe rollback must only query, never mutate outputs"
                );
                f.write("state", reconnect_state.dump());
                manager.cancel_apply();
                report->verify(
                    !manager.has_pending_changes() && f.state()[0]["enabled"] == false,
                    "Reconnect restores originally disabled outputs too"
                );
                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 20000);
        for (const auto & observation : report->observations) {
            QVERIFY2(observation.passed, observation.message.c_str());
        }
        QVERIFY2(result->error.empty(), result->error.c_str());
    }

    void direct_backend() {
        auto fixture = std::make_shared<Fixture>();
        JobDrain drain;
        QVERIFY2(std::setlocale(LC_NUMERIC, "cs_CZ.UTF-8") != nullptr, "Czech locale unavailable");
        auto report = std::make_shared<BackendReport>();
        auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [fixture, report, result] {
                auto & f = *fixture;

                f.reset();
                auto target = f.target();
                WlrRandrBackend backend;
                backend.apply(target);
                report->throws([&] { backend.apply(target); });
                backend.revert();
                report->verify(
                    !backend.has_pending_changes() && f.state()[0]["scale"] == 1,
                    "Direct backend must retain snapshot across repeated Apply"
                );
                backend.apply(target);
                backend.confirm();
                backend.revert();
                report->verify(
                    !backend.has_pending_changes() && f.state()[0]["scale"] == 2,
                    "Direct backend Confirm releases snapshot"
                );

                BackendManager manager;
                manager.apply_with_confirmation(target, nullptr);
                manager.revert();
                manager.set_backend(BackendType::AUTO_WLR_RANDR);
                report->verify(
                    !manager.has_pending_changes(),
                    "Resolved direct transaction must not leak into profile mode"
                );


                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 20000);
        for (const auto & observation : report->observations) {
            QVERIFY2(observation.passed, observation.message.c_str());
        }
        QVERIFY2(result->error.empty(), result->error.c_str());
    }

    void profile_backend_data() {
        QTest::addColumn<int>("backend_type");
        QTest::newRow("kanshi") << static_cast<int>(BackendType::KANSHI);
        QTest::newRow("auto-wlr-randr") << static_cast<int>(BackendType::AUTO_WLR_RANDR);
    }

    void profile_backend() {
        QFETCH(int, backend_type);
        const auto type = static_cast<BackendType>(backend_type);
        auto fixture = std::make_shared<Fixture>();
        JobDrain drain;
        QVERIFY2(std::setlocale(LC_NUMERIC, "cs_CZ.UTF-8") != nullptr, "Czech locale unavailable");
        auto report = std::make_shared<BackendReport>();
        auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [fixture, report, result, type] {
                auto & f = *fixture;

                auto target = f.target();
                BackendManager manager;

                auto display_backend = create_display_backend(type);
                report->verify(
                    !display_backend->has_pending_changes() && !display_backend->can_confirm(),
                    "New backend must start without a pending transaction"
                );
                display_backend->apply(target);
                report->verify(
                    display_backend->has_pending_changes() && display_backend->can_confirm(),
                    "Every backend must report pending state after Apply"
                );
                display_backend->confirm();
                report->verify(
                    !display_backend->has_pending_changes() && !display_backend->can_confirm(),
                    "Confirmation must clear backend pending state"
                );
                display_backend->apply(target);
                display_backend->revert();
                report->verify(
                    !display_backend->has_pending_changes(),
                    "Revert must clear backend pending state"
                );

                manager.set_backend(type);
                const auto calls = f.read("calls");
                manager.apply_with_confirmation(target, nullptr);
                report->verify(
                    manager.has_pending_changes() && manager.can_confirm(),
                    "Manager must delegate pending/confirm state for every backend"
                );
                report->throws([&] { manager.apply_with_confirmation(target, nullptr); });
                report->throws([&] { manager.set_backend(BackendType::WLR_RANDR); });
                manager.confirm_apply();
                report->verify(
                    !manager.has_pending_changes() && !manager.can_confirm(),
                    "Manager confirmation must clear pending state"
                );
                manager.apply_with_confirmation(target, nullptr);
                manager.cancel_apply();
                report->verify(
                    !manager.has_pending_changes() && !manager.can_confirm(),
                    "Manager cancellation must clear pending state"
                );
                manager.apply_with_confirmation(target, nullptr);
                manager.revert();
                report->verify(
                    !manager.has_pending_changes(), "Direct manager revert must clear pending state"
                );
                report->verify(
                    f.read("calls") == calls,
                    "Profile-mode placeholders must not invoke monitor tools"
                );

                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 20000);
        for (const auto & observation : report->observations) {
            QVERIFY2(observation.passed, observation.message.c_str());
        }
        QVERIFY2(result->error.empty(), result->error.c_str());
    }

    void return_to_direct() {
        auto fixture = std::make_shared<Fixture>();
        JobDrain drain;
        QVERIFY2(std::setlocale(LC_NUMERIC, "cs_CZ.UTF-8") != nullptr, "Czech locale unavailable");
        auto report = std::make_shared<BackendReport>();
        auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [fixture, report, result] {
                auto & f = *fixture;

                auto target = f.target();
                BackendManager manager;
                manager.set_backend(BackendType::AUTO_WLR_RANDR);
                manager.set_backend(BackendType::WLR_RANDR);
                manager.apply_with_confirmation(target, nullptr);
                report->verify(
                    manager.has_pending_changes(), "Direct Apply must still create a transaction"
                );
                manager.cancel_apply();
                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 20000);
        for (const auto & observation : report->observations) {
            QVERIFY2(observation.passed, observation.message.c_str());
        }
        QVERIFY2(result->error.empty(), result->error.c_str());
    }
};
QTEST_GUILESS_MAIN(BackendTransactions)
#include "backend_transactions.moc"
