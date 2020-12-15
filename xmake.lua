add_rules("mode.debug", "mode.release")

target("renderer")
    set_kind("binary")
    add_files("src/*.cpp")
    set_rundir("./")