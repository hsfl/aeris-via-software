Import("env")

# Force use of libgcc_nano which doesn't have the large exception tables
env.Append(
    LINKFLAGS=[
        "-specs=nano.specs",
        "-specs=nosys.specs"
    ]
)
