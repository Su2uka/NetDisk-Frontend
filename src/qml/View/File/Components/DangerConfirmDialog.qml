import QtQuick 2.15
import FluentUI 1.0

FluContentDialog {
    id: root

    title: "确认操作"
    message: ""
    buttonFlags: FluContentDialogType.NegativeButton | FluContentDialogType.PositiveButton
    negativeText: "再想想"
    positiveText: "确认"

    property string pendingAction: ""
    property var pendingPayload: null

    signal confirmed(string action, var payload)

    function openDialog(dialogTitle, dialogMessage, confirmText, action, payload) {
        title = dialogTitle;
        message = dialogMessage;
        positiveText = confirmText || "确认";
        pendingAction = action || "";
        pendingPayload = payload || null;
        open();
    }

    onPositiveClicked: {
        root.confirmed(pendingAction, pendingPayload);
        pendingAction = "";
        pendingPayload = null;
    }

    onNegativeClicked: {
        pendingAction = "";
        pendingPayload = null;
    }
}
