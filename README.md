# async_message_queue

Async capabilities for boost::interprocess message_queue.

---

# Dependencies

- Dependencies are managed by *conan*:

```
conan install . -s arch=x86 -s build_type=Debug --build=missing
conan install . -s arch=x86 -s build_type=Release --build=missing
```

## Building

- **Manual build:** You can build this project manually from *Visual Studio*
    - Open the the [project](CMakeLists.txt)
    - Build > Build All
    - Build > Install

    **Note:** Install step not needed when using **editable** mode.
    **Warning:** Make sure the *conan* has been generated in advance.

# Tests

Check the dedicated documentation inside the [tests](/tests) directory.
