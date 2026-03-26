import QtQuick 2.15

QtObject {
    readonly property int app_width: 900
    readonly property int app_height: 600

    readonly property int mobile_app_width: 390
    readonly property int mobile_app_height: 780

    readonly property int margin_small: 6
    readonly property int margin_medium: 8
    readonly property int margin_large: 12

    readonly property int spacing_small: 6
    readonly property int spacing_medium: 8
    readonly property int spacing_large: 12

    readonly property int point_size_small: 14
    readonly property int point_size_medium: 18
    readonly property int point_size_large: 20
    readonly property int point_size_xlarge: 26

    readonly property int sidebar_width: 260
    readonly property int compact_panel_height: 80
    readonly property int standard_panel_height: 160
    readonly property int journal_list_height: 180
    readonly property int form_text_area_height: 60
    readonly property int notes_text_area_height: 80

    readonly property int thumbnail_size_medium: 128
    readonly property int image_tile_height: 160
    readonly property int detail_image_height: 180

    readonly property int popup_width: 380
    readonly property int popup_height: 520
}
