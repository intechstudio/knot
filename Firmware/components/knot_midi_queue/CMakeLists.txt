idf_component_register(
    SRCS
        "knot_midi_queue.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "knot_midi_translator"
)
