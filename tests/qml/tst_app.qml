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
                harness.fail_restore()
            }
            click(control("cancelButton"))
            if (data.tag === "retry") {
                tryCompare(harness.controller, "operationBusy", false, 20000)
                verify(harness.controller.confirmationPending)
                verify(!control("confirmButton").enabled)
                compare(control("cancelButton").text, "Retry restore")
                verify(control("applyError").visible)
                verify(control("applyError").text.length > 0)
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
