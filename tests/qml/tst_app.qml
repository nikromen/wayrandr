import QtQuick
import QtQuick.Controls
import QtTest

TestCase {
    name: "ApplicationFlows"
    when: windowShown
    property var appWindow: null

    function init() {
        harness.begin()
        tryCompare(harness.controller, "operationBusy", false, 20000)
        const component = Qt.createComponent("qrc:/qml/AppWindow.qml")
        compare(component.status, Component.Ready, component.errorString())
        appWindow = component.createObject(null, {width: 1200, height: 800})
        verify(appWindow !== null)
        tryCompare(harness.controller, "operationBusy", false, 20000)
        wait(50)
    }

    function cleanup() {
        if (appWindow) {
            appWindow.destroy()
            appWindow = null
            wait(0)
        }
        const warnings = harness.warnings
        harness.end()
        compare(warnings.length, 0, warnings.join("\n"))
    }

    function control(name) {
        if (name === "backendDiscardDialog") {
            const dialog = findChild(control("wlrBackend").parent, name)
            verify(dialog !== null, name)
            return dialog
        }
        function visibleChild(item) {
            if (item.objectName === name && item.visible) {
                return item
            }
            if (item.children) {
                for (let i = 0; i < item.children.length; ++i) {
                    const found = visibleChild(item.children[i])
                    if (found) {
                        return found
                    }
                }
            }
            return null
        }
        const result = visibleChild(appWindow.contentItem)
        // Hidden transaction controls also have observable visibility/enabled state.
        if (result === null) {
            const hidden = findChild(appWindow.contentItem, name)
            verify(hidden !== null, name)
            return hidden
        }
        return result
    }

    function click(item) {
        verify(waitForRendering(item))
        mouseClick(item)
    }

    function editScale() {
        const spin = control("scaleSpinBox")
        verify(spin.visible && spin.enabled)
        spin.forceActiveFocus()
        keyClick(Qt.Key_Up)
    }

    function test_apply_data() {
        return [{tag: "confirm"}, {tag: "cancel"}, {tag: "retry"}, {tag: "close"}]
    }

    function test_profile_scroll_data() {
        return [{tag: "kanshi", mode: 1}, {tag: "auto", mode: 2}]
    }

    function test_profile_scroll(data) {
        appWindow.width = 640
        appWindow.height = 480
        harness.controller.set_backend_mode(data.mode)
        tryCompare(harness.controller, "operationBusy", false, 20000)
        const editor = harness.controller.profileEditor
        verify(editor.select_profile("desk"))
        for (let i = 0; i < 10; ++i) {
            editor.add_exec_command("notify-send test")
        }
        const scroller = control("profileFormScroll")
        const save = control("profileSaveButton")
        verify(waitForRendering(scroller))
        verify(scroller.contentHeight > scroller.availableHeight)
        scroller.forceActiveFocus()
        for (let i = 0; i < 80 && !save.activeFocus; ++i) {
            keyClick(Qt.Key_Tab)
        }
        verify(save.activeFocus, "Save must be reachable by Tab")
        keyClick(Qt.Key_Tab, Qt.ShiftModifier)
        verify(!save.activeFocus)
        keyClick(Qt.Key_Tab)
        verify(save.activeFocus)
        for (let i = 0; i < 10; ++i) {
            keyClick(Qt.Key_Down)
        }
        tryVerify(function() {
            const position = save.mapToItem(scroller, 0, 0)
            return position.y >= 0 && position.y + save.height <= scroller.height
        })
        keyClick(Qt.Key_Space)
        tryCompare(harness.controller, "operationBusy", false, 20000)
        verify(!editor.isDirty)
        verify(editor.saveStatusText.includes("saved to disk"))
    }

    function test_position_coordinates_data() {
        return [{tag: "wlr", button: "wlrBackend", mode: 0},
                {tag: "kanshi", button: "kanshiBackend", mode: 1},
                {tag: "auto", button: "autoBackend", mode: 2}]
    }

    function test_position_coordinates(data) {
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        let output = harness.controller.monitors[0]
        if (data.mode !== 0) {
            verify(harness.controller.profileEditor.select_profile("desk"))
            output = harness.controller.profileEditor.outputs[0]
        }
        const xSpin = control("positionXSpinBox")
        const ySpin = control("positionYSpinBox")
        for (const position of [{x: 80, y: 50}, {x: -1920, y: -1080},
                                {x: 1000001, y: 1000002}]) {
            output.positionX = position.x
            output.positionY = position.y
            compare(output.positionX, position.x)
            compare(output.positionY, position.y)
            compare(xSpin.value, position.x)
            compare(ySpin.value, position.y)
        }
        output.positionX = -1920
        xSpin.forceActiveFocus()
        keyClick(Qt.Key_Up)
        compare(output.positionX, -1919)
        compare(output.positionY, 1000002)
    }

    function test_apply(data) {
        editScale()
        compare(harness.controller.monitors[0].scale.toFixed(1), "1.1")
        click(control("applyButton"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        tryCompare(harness.controller, "confirmationPending", true)
        compare(harness.scale().toFixed(1), "1.1")
        verify(!control("applyButton").visible)
        verify(control("confirmButton").enabled)
        verify(control("cancelButton").visible)
        if (data.tag === "confirm") {
            click(control("confirmButton"))
        } else if (data.tag === "close") {
            appWindow.close()
        } else {
            if (data.tag === "retry") {
                harness.fail_restore("<b>restore failure</b>")
            }
            click(control("cancelButton"))
            if (data.tag === "retry") {
                tryCompare(harness.controller, "operationBusy", false, 20000)
                verify(harness.controller.confirmationPending)
                verify(!control("confirmButton").enabled)
                compare(control("cancelButton").text, "Retry restore")
                verify(control("applyError").visible)
                verify(control("applyError").text.includes("<b>restore failure</b>"))
                compare(control("applyError").textFormat, Text.PlainText)
                click(control("cancelButton"))
            }
        }
        tryCompare(harness.controller, "operationBusy", false, 20000)
        tryCompare(harness.controller, "confirmationPending", false)
        if (data.tag === "confirm") {
            compare(harness.scale().toFixed(1), "1.1")
        } else {
            compare(harness.scale(), 1)
        }
        if (data.tag === "close") {
            tryCompare(appWindow, "visible", false)
        } else {
            verify(control("applyButton").visible && control("applyButton").enabled)
        }
    }

    function test_refreshed_modes_data() {
        return [{tag: "changed", empty: false}, {tag: "empty", empty: true},
                {tag: "disabled", disabled: true},
                {tag: "disconnected", disconnected: true}]
    }

    function test_refreshed_modes(data) {
        const monitor = harness.controller.monitors[0]
        if (data.disabled) {
            monitor.scale = 1.5
        }
        click(control("applyButton"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        verify(harness.controller.confirmationPending)
        const combo = control("resolutionCombo")
        compare(combo.count, 1)
        if (data.disconnected) {
            harness.disconnect_output()
        } else {
            harness.change_modes(!!data.empty, !!data.disabled)
        }
        click(control("confirmButton"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        compare(harness.controller.monitors[0], monitor)
        if (data.disconnected) {
            verify(!monitor.enabled)
            verify(!monitor.hasSettings)
            compare(monitor.resolutionWidth, 0)
            compare(monitor.resolutionHeight, 0)
            compare(monitor.activeResolutionIndex, 0)
            verify(!combo.visible)
            compare(combo.currentIndex, -1)
        } else if (data.disabled) {
            verify(!monitor.hasSettings)
            monitor.enabled = true
            compare(monitor.activeResolutionIndex, 1)
            compare(monitor.resolutionWidth, 1920)
            compare(combo.currentIndex, 1)
            compare(monitor.scale, 1.5)
        } else if (data.empty) {
            compare(monitor.resolutions.length, 0)
            compare(combo.count, 0)
            compare(combo.currentIndex, -1)
            compare(monitor.resolutionWidth, 0)
        } else {
            compare(monitor.resolutions.length, 2)
            compare(combo.count, 2)
            compare(combo.currentIndex, 1)
            compare(monitor.activeResolutionIndex, 1)
            compare(monitor.resolutionWidth, 2560)
            compare(combo.currentText, monitor.resolutions[1])
            combo.forceActiveFocus()
            keyClick(Qt.Key_Home)
            compare(monitor.activeResolutionIndex, 0)
            compare(monitor.resolutionWidth, 1280)
        }
    }

    function test_unsaved_profile_data() {
        return [{tag: "kanshi", button: "kanshiBackend", mode: 1},
                {tag: "auto", button: "autoBackend", mode: 2}]
    }

    function test_unsaved_profile(data) {
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        compare(harness.controller.backendMode, data.mode)
        const editor = harness.controller.profileEditor
        verify(editor.select_profile("desk"))
        // Both profile views exist; choose the visible scale editor.
        verify(!editor.isDirty)
        const spins = []
        function collect(item) {
            if (item.objectName === "scaleSpinBox" && item.visible) {
                spins.push(item)
            }
            if (item.children) {
                for (let i = 0; i < item.children.length; ++i) {
                    collect(item.children[i])
                }
            }
        }
        collect(appWindow.contentItem)
        compare(spins.length, 1)
        appWindow.requestActivate()
        wait(50)
        spins[0].forceActiveFocus()
        keyClick(Qt.Key_Up)
        verify(editor.isDirty)
        compare(editor.outputs[0].scale.toFixed(1), "1.1")
        const profileCombo = control("profileCombo")
        profileCombo.forceActiveFocus()
        keyClick(Qt.Key_Space)
        tryCompare(profileCombo.popup, "visible", true)
        keyClick(Qt.Key_Return)
        tryCompare(profileCombo.popup, "visible", false)
        compare(editor.selectedProfileId, "desk")
        verify(editor.isDirty)
        compare(editor.outputs[0].scale.toFixed(1), "1.1")
        click(control("wlrBackend"))
        const dialog = control("backendDiscardDialog")
        tryCompare(dialog, "visible", true)
        click(dialog.standardButton(Dialog.No))
        compare(harness.controller.backendMode, data.mode)
        verify(editor.isDirty)
        compare(editor.outputs[0].scale.toFixed(1), "1.1")
        click(control("wlrBackend"))
        tryCompare(dialog, "visible", true)
        click(dialog.standardButton(Dialog.Yes))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        compare(harness.controller.backendMode, 0)
        verify(!editor.isDirty)
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        verify(editor.select_profile("desk"))
        compare(editor.outputs[0].scale, 1)
    }

    function test_unsaved_close_data() {
        return test_unsaved_profile_data()
    }

    function test_unsaved_close(data) {
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        const editor = harness.controller.profileEditor
        verify(editor.select_profile("desk"))
        editor.add_exec_command("notify-send unsaved")
        verify(editor.isDirty)
        appWindow.close()
        const dialog = findChild(appWindow, "closeDiscardDialog")
        verify(dialog !== null)
        tryCompare(dialog, "visible", true)
        click(dialog.standardButton(Dialog.No))
        verify(appWindow.visible)
        verify(editor.isDirty)
        verify(editor.execCommands.indexOf("notify-send unsaved") >= 0)
        appWindow.close()
        tryCompare(dialog, "visible", true)
        click(dialog.standardButton(Dialog.Yes))
        tryCompare(appWindow, "visible", false)
    }
}
