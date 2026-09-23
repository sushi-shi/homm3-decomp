    // Dreamcast header inlines used by mode-switch paths.
    void hide()
    {
        sendMessage(WIDGET_CLEAR_STATUS, WIDGET_ACTIVE | WIDGET_DRAWN);
    }
