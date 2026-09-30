import QtQuick
import QtQuick.Controls
import QtTest
import com.nikromen.wayrandr 1.0

TestCase {
    name: "ApplicationFlows"
    when: windowShown
    property var appWindow: null

    Component {
        id: transformModel
        QtObject {
            property string transform: "normal"
            readonly property var transformList: ["normal", "90", "180", "270", "flipped",
                                                  "flipped-90", "flipped-180", "flipped-270"]
            signal transform_changed()
            onTransformChanged: transform_changed()
        }
    }

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
        if (name === "profileDiscardDialog") {
            let parent = control("profileCombo").parent
            while (parent) {
                const dialog = findChild(parent, name)
                if (dialog !== null) {
                    return dialog
                }
                parent = parent.parent
            }
            fail("Missing profile discard dialog")
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

    function verifyDirectInputsLocked() {
        verify(!harness.controller.directEditingAllowed)
        const monitor = harness.controller.monitors[0]
        const x = monitor.positionX
        const y = monitor.positionY
        const scale = monitor.scale
        const transform = monitor.transform
        const mode = monitor.activeResolutionIndex
        for (const name of ["scaleSpinBox", "positionXSpinBox", "positionYSpinBox",
                            "transformComboBox", "resolutionCombo"]) {
            const item = control(name)
            verify(!item.enabled, name)
            item.forceActiveFocus()
            keyClick(Qt.Key_Up)
        }
        const reset = control("resetLayoutButton")
        verify(!reset.enabled)
        click(reset)
        const block = control("monitorCanvasBlock")
        verify(!block.enabled)
        verify(waitForRendering(block))
        mouseDrag(block, block.width / 2, block.height / 2, 40, 30)
        compare(monitor.scale, scale)
        compare(monitor.positionX, x)
        compare(monitor.positionY, y)
        compare(monitor.transform, transform)
        compare(monitor.activeResolutionIndex, mode)
        compare(control("scaleSpinBox").value, Math.round(scale * 10))
        compare(control("positionXSpinBox").value, x)
        compare(control("positionYSpinBox").value, y)
        compare(control("transformComboBox").currentText, transform)
    }

    function test_apply_data() {
        return [{tag: "confirm"}, {tag: "cancel"}, {tag: "retry"}, {tag: "close"}]
    }

    function chooseTransform(combo, transform) {
        const index = combo.model.indexOf(transform)
        verify(index >= 0)
        combo.forceActiveFocus()
        keyClick(Qt.Key_Space)
        tryCompare(combo.popup, "visible", true)
        keyClick(Qt.Key_Home)
        for (let i = 0; i < index; ++i) {
            keyClick(Qt.Key_Down)
        }
        keyClick(Qt.Key_Return)
        tryCompare(combo.popup, "visible", false)
        compare(combo.currentText, transform)
        compare(combo.item.transform, transform)
    }

    function test_transform_item_lifetime() {
        const first = createTemporaryObject(transformModel, appWindow, {transform: "90"})
        const second = createTemporaryObject(transformModel, appWindow, {transform: "180"})
        const component = Qt.createComponent("qrc:/qml/TransformComboBox.qml")
        compare(component.status, Component.Ready, component.errorString())
        const combo = createTemporaryObject(component, appWindow.contentItem,
                                           {item: first, width: 200, height: 40})
        compare(combo.currentText, "90")
        combo.item = second
        compare(combo.currentText, "180")
        compare(first.transform, "90")
        compare(second.transform, "180")
        first.transform = "flipped"
        compare(combo.currentText, "180")
        combo.item = null
        compare(combo.currentIndex, -1)
        compare(combo.currentText, "")
        combo.item = undefined
        compare(combo.currentIndex, -1)
        compare(combo.currentText, "")
        combo.item = second
        compare(combo.currentText, "180")
        second.destroy()
        tryCompare(combo, "item", null)
        compare(combo.currentIndex, -1)
        compare(combo.currentText, "")
        combo.item = first
        compare(combo.currentText, "flipped")
        first.transform = "270"
        compare(combo.currentText, "270")
        chooseTransform(combo, "flipped-180")
        const empty = createTemporaryObject(component, appWindow.contentItem)
        compare(empty.currentIndex, -1)
        compare(empty.currentText, "")
    }

    function test_transform_tabs_data() {
        return test_unsaved_profile_data()
    }

    function chooseOutputTab(tabs, index) {
        tabs.itemAt(index).forceActiveFocus()
        keyClick(Qt.Key_Space)
        tryCompare(tabs, "currentIndex", index)
    }

    function test_transform_tabs(data) {
        harness.transform_profiles()
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        const editor = harness.controller.profileEditor
        const tabs = control("outputTabBar")
        const combo = control("transformComboBox")
        const scale = control("scaleSpinBox")
        const storedScale = editor.outputs[0].scale
        scale.to = 8
        compare(scale.value, 8)
        compare(editor.outputs[0].scale, storedScale)
        verify(!editor.isDirty)
        scale.to = 150
        chooseOutputTab(tabs, 1)
        const scales = [12, 150, 10, 10]
        const transforms = ["90", "180", "flipped-90", "normal"]
        compare(editor.outputs.length, transforms.length)
        for (const index of [0, 1, 2, 0, 3]) {
            chooseOutputTab(tabs, index)
            tryCompare(combo, "item", editor.outputs[index])
            compare(combo.currentText, transforms[index])
            compare(combo.item.transform, transforms[index])
            compare(scale.value, scales[index])
            compare(harness.has_transform(editor.outputs[index]), index !== 3)
            verify(!editor.isDirty)
        }
        scale.forceActiveFocus()
        keyClick(Qt.Key_Up)
        compare(editor.outputs[3].scale.toFixed(1), "1.1")
        compare(editor.outputs[0].scale, storedScale)
        compare(editor.outputs[1].scale, 20)
        chooseTransform(combo, "flipped-270")
        verify(editor.isDirty)
        verify(harness.has_transform(editor.outputs[3]))
        for (let i = 0; i < 3; ++i) {
            compare(editor.outputs[i].transform, transforms[i])
        }
        editor.outputs[3].transform = "flipped-180"
        compare(combo.currentText, "flipped-180")
        editor.outputs[3].scale = 1.45
        compare(scale.value, 15)
        compare(editor.outputs[3].scale.toFixed(2), "1.45")
        const old = combo.item
        activateProfileButton("profileDiscardButton")
        verify(combo.item !== old)
        compare(combo.item, editor.outputs[0])
        compare(combo.currentText, "90")
        compare(scale.value, 12)
        verify(!editor.isDirty)
        chooseOutputTab(tabs, 3)
        compare(combo.currentText, "normal")
        compare(scale.value, 10)
        verify(!harness.has_transform(editor.outputs[3]))
        chooseOutputTab(tabs, 2)
        compare(combo.currentText, "flipped-90")
        editor.reload_config_from_disk()
        compare(combo.item, editor.outputs[0])
        compare(combo.currentText, "90")
        verify(!harness.has_transform(editor.outputs[3]))
        verify(!editor.isDirty)
    }

    function test_profile_programmatic_controls() {
        click(control("autoBackend"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        const editor = harness.controller.profileEditor
        const output = editor.outputs[0]
        const enabled = control("outputEnabledSwitch")
        const adaptive = control("adaptiveSyncSwitch")
        const preferred = control("preferredSwitch")
        const mode = control("profileModeField")
        const storedMode = output.mode
        enabled.checked = false
        adaptive.checked = true
        preferred.checked = true
        mode.text = "programmatic text"
        verify(output.enabled)
        verify(!output.adaptiveSync)
        verify(!output.preferred)
        compare(output.mode, storedMode)
        verify(!editor.isDirty)
        // Restore the read bindings deliberately replaced by the assignments above.
        enabled.checked = Qt.binding(function() { return output.enabled })
        adaptive.checked = Qt.binding(function() { return output.adaptiveSync })
        preferred.checked = Qt.binding(function() { return output.preferred })
        mode.text = Qt.binding(function() { return output.mode })
        preferred.forceActiveFocus()
        keyClick(Qt.Key_Space)
        verify(output.preferred)
        mode.forceActiveFocus()
        keyClick(Qt.Key_A, Qt.ControlModifier)
        keyClick(Qt.Key_Backspace)
        compare(output.mode, "")
        enabled.forceActiveFocus()
        keyClick(Qt.Key_Space)
        verify(!output.enabled)
        adaptive.forceActiveFocus()
        keyClick(Qt.Key_Space)
        verify(output.adaptiveSync)
        verify(editor.isDirty)
        output.enabled = true
        verify(enabled.checked)
    }

    function test_profile_scroll_data() {
        return [{tag: "kanshi", mode: EditorMode.KANSHI},
                {tag: "auto", mode: EditorMode.AUTO_WLR_RANDR}]
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
        return [{tag: "wlr", button: "wlrBackend", mode: EditorMode.WLR_RANDR},
                {tag: "kanshi", button: "kanshiBackend", mode: EditorMode.KANSHI},
                {tag: "auto", button: "autoBackend", mode: EditorMode.AUTO_WLR_RANDR}]
    }

    function test_unusable_profile_scale_data() {
        return [{tag: "nan", scale: NaN}, {tag: "infinity", scale: Infinity},
                {tag: "negative-infinity", scale: -Infinity}, {tag: "zero", scale: 0},
                {tag: "negative", scale: -1}, {tag: "tiny", scale: 1e-30}]
    }

    function test_unusable_profile_scale(data) {
        click(control("kanshiBackend"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        const editor = harness.controller.profileEditor
        verify(editor.select_profile("desk"))
        const output = editor.outputs[0]
        output.scale = data.scale
        const stored = output.scale
        wait(0)
        compare(output.layoutWidth, 0)
        compare(output.layoutHeight, 0)
        const spin = control("scaleSpinBox")
        compare(spin.value, spin.from)
        if (isNaN(stored)) {
            verify(isNaN(output.scale))
        } else {
            compare(output.scale, stored)
        }
    }

    function test_position_coordinates(data) {
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        let output = harness.controller.monitors[0]
        if (data.mode !== EditorMode.WLR_RANDR) {
            verify(harness.controller.profileEditor.select_profile("desk"))
            output = harness.controller.profileEditor.outputs[0]
        }
        const xSpin = control("positionXSpinBox")
        const ySpin = control("positionYSpinBox")
        let formName = "profileOutputProperties"
        let propertyName = "output"
        if (data.mode === EditorMode.WLR_RANDR) {
            verify(!output.dirty)
            formName = "monitorProperties"
            propertyName = "monitor"
        } else {
            verify(!harness.controller.profileEditor.isDirty)
        }
        const form = control(formName)
        const scale = control("scaleSpinBox")
        for (const item of [null, undefined]) {
            form[propertyName] = item
            compare(xSpin.value, 0)
            compare(ySpin.value, 0)
            compare(scale.value, scale.from)
            verify(!scale.enabled)
        }
        form[propertyName] = output
        compare(xSpin.value, output.positionX)
        compare(ySpin.value, output.positionY)
        for (const position of [{x: 80, y: 50}, {x: -1920, y: -1080},
                                {x: -2147483648, y: 2147483647},
                                {x: 2147483647, y: -2147483648},
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
        const storedX = output.positionX
        const storedY = output.positionY
        xSpin.value = 42
        ySpin.value = -32
        compare(output.positionX, storedX)
        compare(output.positionY, storedY)
    }

    function test_apply(data) {
        const monitor = harness.controller.monitors[0]
        monitor.positionX = 240
        monitor.positionY = 120
        editScale()
        compare(harness.controller.monitors[0].scale.toFixed(1), "1.1")
        click(control("applyButton"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        tryCompare(harness.controller, "confirmationPending", true)
        compare(harness.scale().toFixed(1), "1.1")
        verifyDirectInputsLocked()
        compare(monitor.scale.toFixed(1), "1.1")
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
                verifyDirectInputsLocked()
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
            verify(harness.controller.directEditingAllowed)
            const scale = monitor.scale
            editScale()
            compare(monitor.scale.toFixed(1), (scale + 0.1).toFixed(1))
        }
    }

    function test_inflight_direct_edits() {
        editScale()
        harness.hold_command("mutation")
        click(control("applyButton"))
        tryVerify(function() { return harness.command_entered("mutation") }, 10000)
        verify(harness.controller.operationBusy)
        compare(harness.scale().toFixed(1), "1.1")
        verifyDirectInputsLocked()
        harness.controller.monitors[0].scale = 1.2
        compare(harness.controller.monitors[0].scale.toFixed(1), "1.1")
        harness.release_command("mutation")
        tryCompare(harness.controller, "operationBusy", false, 20000)
        tryCompare(harness.controller, "confirmationPending", true)
        harness.hold_command("query")
        click(control("confirmButton"))
        tryVerify(function() { return harness.command_entered("query") }, 10000)
        verify(harness.controller.operationBusy)
        verify(!harness.controller.confirmationPending)
        verifyDirectInputsLocked()
        harness.release_command("query")
        tryCompare(harness.controller, "operationBusy", false, 20000)
        verify(harness.controller.directEditingAllowed)
        editScale()
        compare(harness.controller.monitors[0].scale.toFixed(1), "1.2")
        const monitor = harness.controller.monitors[0]
        const block = control("monitorCanvasBlock")
        verify(block.enabled)
        mouseDrag(block, block.width / 2, block.height / 2, 40, 30)
        verify(monitor.positionX !== 0 || monitor.positionY !== 0)
        click(control("resetLayoutButton"))
        compare(monitor.positionX, 0)
        compare(monitor.positionY, 0)
        compare(harness.scale().toFixed(1), "1.1")
    }

    function test_output_set_data() {
        return [{tag: "kanshi", button: "kanshiBackend"},
                {tag: "auto", button: "autoBackend"}]
    }

    function test_output_set(data) {
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        harness.output_set(["DP-1", "DP-2"], 1)
        click(control("wlrBackend"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        const first = harness.controller.monitors[0]
        const second = harness.controller.monitors[1]
        compare(first.name, "DP-1")
        compare(second.name, "DP-2")
        click(control("monitorTab_DP-2"))
        compare(control("monitorTabBar").currentIndex, 1)
        compare(control("scaleSpinBox").value, 20)
        compare(control("transformComboBox").currentText, "flipped-90")
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        harness.output_set(["DP-2", "DP-NEW", "DP-1"], 4)
        click(control("wlrBackend"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        tryCompare(control("monitorTabBar"), "currentIndex", 0)
        compare(harness.controller.monitors[0], second)
        compare(harness.controller.monitors[2], first)
        compare(control("scaleSpinBox").value, 40)
        const added = harness.controller.monitors[1]
        verify(!first.dirty && !second.dirty && !added.dirty)
        compare(added.name, "DP-NEW")
        verify(harness.controller.get_monitor_block(added.name) !== null)
        function findBlock(item) {
            if (item.objectName === "monitorCanvasBlock" && item.name === "DP-NEW") {
                return item
            }
            if (item.children) {
                for (const child of item.children) {
                    const result = findBlock(child)
                    if (result) {
                        return result
                    }
                }
            }
            return null
        }
        const block = findBlock(appWindow.contentItem)
        verify(block !== null && block.visible && block.enabled)
        click(block)
        compare(control("monitorTabBar").currentIndex, 1)
        compare(control("scaleSpinBox").value, 50)
        chooseTransform(control("transformComboBox"), "270")
        compare(added.transform, "270")
        compare(second.transform, "flipped-90")
        verify(added.dirty)
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        harness.output_set(["DP-1"], 7)
        click(control("wlrBackend"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        tryCompare(control("monitorTabBar"), "currentIndex", 0)
        compare(harness.controller.monitors[0], first)
        compare(control("scaleSpinBox").value, 70)
        verify(!second.connected && !added.connected)
        verify(!control("monitorTab_DP-2").enabled)
        verify(!control("monitorTab_DP-NEW").enabled)
        verify(added.dirty)
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        harness.output_set([], 0)
        click(control("wlrBackend"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        tryCompare(control("monitorTabBar"), "currentIndex", -1)
        verify(!first.connected)
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        harness.output_set(["DP-NEW", "DP-1", "DP-2"], 10)
        click(control("wlrBackend"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        tryCompare(control("monitorTabBar"), "currentIndex", 0)
        compare(harness.controller.monitors[0], added)
        compare(harness.controller.monitors[1], first)
        compare(harness.controller.monitors[2], second)
        verify(added.connected)
        compare(added.transform, "270")
        compare(control("transformComboBox").currentText, "270")
        compare(control("scaleSpinBox").value, 50)
        compare(harness.scale(), 10)
        editScale()
        compare(added.scale.toFixed(1), "5.1")
        compare(harness.scale(), 10)
    }

    function test_return_refresh_error_and_draft() {
        const monitor = harness.controller.monitors[0]
        editScale()
        click(control("kanshiBackend"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        harness.output_set(["DP-1"], 2)
        harness.hold_command("query")
        click(control("wlrBackend"))
        tryVerify(function() { return harness.command_entered("query") }, 10000)
        verifyDirectInputsLocked()
        harness.controller.monitors[0].scale = 1.2
        compare(monitor.scale.toFixed(1), "1.1")
        harness.fail_query(true)
        harness.release_command("query")
        tryCompare(harness.controller, "operationBusy", false, 20000)
        compare(harness.controller.monitors[0], monitor)
        compare(monitor.scale.toFixed(1), "1.1")
        verify(monitor.connected && monitor.dirty)
        verify(control("applyError").text.indexOf("Could not refresh monitors") >= 0)
        editScale()
        compare(monitor.scale.toFixed(1), "1.2")
        compare(harness.scale(), 2)
        harness.fail_query(false)
        click(control("kanshiBackend"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        harness.output_set(["DP-1"], 3)
        harness.hold_command("query")
        click(control("wlrBackend"))
        tryVerify(function() { return harness.command_entered("query") }, 10000)
        verifyDirectInputsLocked()
        harness.release_command("query")
        tryCompare(harness.controller, "operationBusy", false, 20000)
        compare(control("scaleSpinBox").value, 12)
        compare(control("applyError").text, "")
        editScale()
        compare(monitor.scale.toFixed(1), "1.3")
        compare(harness.scale(), 3)
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
        return [{tag: "kanshi", button: "kanshiBackend", mode: EditorMode.KANSHI},
                {tag: "auto", button: "autoBackend", mode: EditorMode.AUTO_WLR_RANDR}]
    }

    function test_unsaved_profile(data) {
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        compare(harness.controller.backendMode, data.mode)
        const editor = harness.controller.profileEditor
        verify(editor.select_profile("desk"))
        verify(!editor.isDirty)
        appWindow.requestActivate()
        wait(50)
        editScale()
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
        compare(harness.controller.backendMode, EditorMode.WLR_RANDR)
        verify(!editor.isDirty)
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        verify(editor.select_profile("desk"))
        compare(editor.outputs[0].scale, 1)

        if (data.mode === EditorMode.KANSHI) {
            // Switch documents while an obsolete profile-selection dialog is still open.
            const transformCombo = control("transformComboBox")
            editScale()
            chooseProfile("spare")
            const profileDialog = control("profileDiscardDialog")
            tryCompare(profileDialog, "visible", true)
            editor.discard_changes()
            harness.empty_auto_config()
            harness.controller.set_backend_mode(EditorMode.AUTO_WLR_RANDR)
            tryCompare(harness.controller, "operationBusy", false, 20000)
            compare(harness.controller.backendMode, EditorMode.AUTO_WLR_RANDR)
            verify(!profileDialog.visible)
            compare(profileDialog.pendingProfileId, "")
            compare(profileDialog.pendingAction, 0)
            compare(profileCombo.currentIndex, -1)
            compare(editor.outputs.length, 0)
            compare(transformCombo.item, null)
            verify(control("onNoMatchCommandEditor").visible)
            verify(editor.configurationLoaded && !editor.isDirty)
            click(control("kanshiBackend"))
            tryCompare(harness.controller, "operationBusy", false, 20000)
            checkProfile("desk", 1)
        }
    }

    function test_unsaved_close_data() {
        return test_unsaved_profile_data()
    }

    function chooseProfile(id) {
        const combo = control("profileCombo")
        const index = combo.model.indexOf(id)
        verify(index >= 0, id)
        combo.forceActiveFocus()
        keyClick(Qt.Key_Space)
        tryCompare(combo.popup, "visible", true)
        keyClick(Qt.Key_Home)
        for (let i = 0; i < index; ++i) {
            keyClick(Qt.Key_Down)
        }
        keyClick(Qt.Key_Return)
        tryCompare(combo.popup, "visible", false)
        compare(harness.warnings.length, 0, harness.warnings.join("\n"))
    }

    function checkProfile(id, scale) {
        const editor = harness.controller.profileEditor
        compare(editor.selectedProfileId, id)
        compare(control("profileCombo").currentText, id)
        compare(editor.outputs.length, 1)
        compare(editor.outputs[0].scale.toFixed(1), scale.toFixed(1))
        compare(control("scaleSpinBox").value, Math.round(scale * 10))
    }

    function activateProfileButton(name) {
        const button = control(name)
        verify(button.enabled)
        button.forceActiveFocus()
        keyClick(Qt.Key_Space)
        tryCompare(harness.controller, "operationBusy", false, 20000)
    }

    function test_profile_selection_data() {
        const rows = []
        for (const backend of test_unsaved_profile_data()) {
            for (const action of ["no", "yes", "close", "escape"]) {
                rows.push({tag: backend.tag + "-" + action, button: backend.button,
                           mode: backend.mode, action: action})
            }
        }
        return rows
    }

    function test_profile_selection(data) {
        harness.switching_profiles(true)
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        const editor = harness.controller.profileEditor
        checkProfile("desk", 1)
        editScale()
        verify(editor.isDirty)
        chooseProfile("spare")
        const dialog = control("profileDiscardDialog")
        tryCompare(dialog, "visible", true)
        checkProfile("desk", 1.1)
        verify(editor.isDirty)

        if (data.action === "close") {
            dialog.close()
        } else if (data.action === "escape") {
            keyClick(Qt.Key_Escape)
        } else if (data.action === "no") {
            click(dialog.standardButton(Dialog.No))
        } else {
            click(dialog.standardButton(Dialog.Yes))
        }
        tryCompare(dialog, "visible", false)

        if (data.action === "close" || data.action === "escape") {
            checkProfile("desk", 1.1)
            verify(editor.isDirty)
            chooseProfile("travel")
            tryCompare(dialog, "visible", true)
            checkProfile("desk", 1.1)
            click(dialog.standardButton(Dialog.Yes))
            tryCompare(dialog, "visible", false)
            checkProfile("travel", 3)
            verify(!editor.isDirty)
            chooseProfile("desk")
            checkProfile("desk", 1)
        } else {
            let savedId = "desk"
            let savedScale = 1.1
            let remainingId = "spare"
            let remainingScale = 2
            if (data.action === "yes") {
                checkProfile("spare", 2)
                verify(!editor.isDirty)
                chooseProfile("desk")
                checkProfile("desk", 1)
                chooseProfile("spare")
                editScale()
                savedId = "spare"
                savedScale = 2.1
                remainingId = "desk"
                remainingScale = 1
            }
            checkProfile(savedId, savedScale)
            verify(editor.isDirty)
            activateProfileButton("profileSaveButton")
            verify(!editor.isDirty)
            verify(editor.saveStatusText.includes("saved to disk"))
            editor.reload_config_from_disk()
            checkProfile(savedId, savedScale)
            activateProfileButton("profileDeleteButton")
            verify(editor.profileIds.indexOf(savedId) < 0)
            checkProfile(remainingId, remainingScale)
            editor.reload_config_from_disk()
            verify(editor.profileIds.indexOf(savedId) < 0)
            checkProfile(remainingId, remainingScale)
        }
        // Clean changes and reactivation of the selected item need no discard dialog.
        chooseProfile("travel")
        checkProfile("travel", 3)
        chooseProfile("travel")
        checkProfile("travel", 3)
        verify(!dialog.visible)
        verify(!editor.isDirty)
    }

    function test_profile_reload_data() {
        return test_unsaved_profile_data()
    }

    function test_profile_reload(data) {
        harness.switching_profiles(true)
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        const editor = harness.controller.profileEditor
        editScale()
        chooseProfile("travel")
        const dialog = control("profileDiscardDialog")
        tryCompare(dialog, "visible", true)
        dialog.close()
        tryCompare(dialog, "visible", false)
        checkProfile("desk", 1.1)
        activateProfileButton("profileDiscardButton")
        checkProfile("desk", 1)
        verify(!editor.isDirty)

        click(control("wlrBackend"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        harness.switching_profiles(false)
        click(control(data.button))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        verify(!editor.configurationLoaded)
        if (data.mode === EditorMode.AUTO_WLR_RANDR) {
            verify(!control("onNoMatchCommandEditor").enabled)
        }
        editor.create_profile_from_live("draft")
        tryCompare(harness.controller, "operationBusy", false, 20000)
        checkProfile("draft", 1)
        verify(editor.isDirty)
        harness.switching_profiles(true)
        activateProfileButton("profileReloadButton")
        tryCompare(dialog, "visible", true)
        click(dialog.standardButton(Dialog.No))
        tryCompare(dialog, "visible", false)
        checkProfile("draft", 1)
        verify(editor.isDirty)
        activateProfileButton("profileReloadButton")
        tryCompare(dialog, "visible", true)
        click(dialog.standardButton(Dialog.Yes))
        tryCompare(dialog, "visible", false)
        verify(editor.configurationLoaded)
        verify(!editor.isDirty)
        checkProfile("desk", 1)
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

    function test_global_commands_without_profile() {
        harness.empty_auto_config()
        click(control("autoBackend"))
        tryCompare(harness.controller, "operationBusy", false, 20000)
        const editor = harness.controller.profileEditor
        verify(editor.configurationLoaded)
        compare(editor.profileIds.length, 0)
        compare(editor.selectedProfileId, "")
        const commands = control("onNoMatchCommandEditor")
        const add = findChild(commands, "addCommandButton")
        add.forceActiveFocus()
        keyClick(Qt.Key_Space)
        compare(editor.onNoMatchExecCommands.length, 1)
        verify(editor.isDirty)
        const field = findChild(commands, "commandField")
        field.forceActiveFocus()
        for (const character of "echo saved") {
            keyClick(character)
        }
        keyClick(Qt.Key_Return)
        compare(editor.onNoMatchExecCommands[0], "echo saved")
        activateProfileButton("profileSaveButton")
        verify(!editor.isDirty)
        editor.reload_config_from_disk()
        compare(editor.onNoMatchExecCommands[0], "echo saved")
        compare(editor.profileIds.length, 0)
        const remove = findChild(commands, "removeCommandButton")
        remove.forceActiveFocus()
        keyClick(Qt.Key_Space)
        verify(editor.isDirty)
        compare(editor.onNoMatchExecCommands.length, 0)
        appWindow.close()
        const dialog = findChild(appWindow, "closeDiscardDialog")
        tryCompare(dialog, "visible", true)
        click(dialog.standardButton(Dialog.No))
        verify(appWindow.visible && editor.isDirty)
        activateProfileButton("profileDiscardButton")
        verify(!editor.isDirty)
        compare(editor.onNoMatchExecCommands[0], "echo saved")
        verify(!control("profileSaveButton").enabled)
        verify(!control("profileDiscardButton").enabled)
    }
}
