TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lboost_thread -lboost_system -lboost_program_options -lpthread -lssl -lcrypto -lz
LIBS += ../DMProxy/lib/libprotobuf.a

INCLUDEPATH += ./include
INCLUDEPATH += ./

DISTFILES += \
    lib/libprotobuf.a \
    LICENSE \
    include/google/protobuf/compiler/plugin.proto \
    include/google/protobuf/compiler/profile.proto \
    include/google/protobuf/any.proto \
    include/google/protobuf/api.proto \
    include/google/protobuf/descriptor.proto \
    include/google/protobuf/duration.proto \
    include/google/protobuf/empty.proto \
    include/google/protobuf/field_mask.proto \
    include/google/protobuf/source_context.proto \
    include/google/protobuf/struct.proto \
    include/google/protobuf/timestamp.proto \
    include/google/protobuf/type.proto \
    include/google/protobuf/wrappers.proto

HEADERS += \
    include/google/protobuf/compiler/cpp/cpp_generator.h \
    include/google/protobuf/compiler/csharp/csharp_generator.h \
    include/google/protobuf/compiler/csharp/csharp_names.h \
    include/google/protobuf/compiler/java/java_generator.h \
    include/google/protobuf/compiler/java/java_names.h \
    include/google/protobuf/compiler/javanano/javanano_generator.h \
    include/google/protobuf/compiler/js/js_generator.h \
    include/google/protobuf/compiler/js/well_known_types_embed.h \
    include/google/protobuf/compiler/objectivec/objectivec_generator.h \
    include/google/protobuf/compiler/objectivec/objectivec_helpers.h \
    include/google/protobuf/compiler/php/php_generator.h \
    include/google/protobuf/compiler/python/python_generator.h \
    include/google/protobuf/compiler/ruby/ruby_generator.h \
    include/google/protobuf/compiler/code_generator.h \
    include/google/protobuf/compiler/command_line_interface.h \
    include/google/protobuf/compiler/importer.h \
    include/google/protobuf/compiler/parser.h \
    include/google/protobuf/compiler/plugin.h \
    include/google/protobuf/compiler/plugin.pb.h \
    include/google/protobuf/compiler/profile.pb.h \
    include/google/protobuf/io/coded_stream.h \
    include/google/protobuf/io/gzip_stream.h \
    include/google/protobuf/io/printer.h \
    include/google/protobuf/io/strtod.h \
    include/google/protobuf/io/tokenizer.h \
    include/google/protobuf/io/zero_copy_stream.h \
    include/google/protobuf/io/zero_copy_stream_impl.h \
    include/google/protobuf/io/zero_copy_stream_impl_lite.h \
    include/google/protobuf/stubs/atomic_sequence_num.h \
    include/google/protobuf/stubs/atomicops.h \
    include/google/protobuf/stubs/atomicops_internals_arm64_gcc.h \
    include/google/protobuf/stubs/atomicops_internals_arm_gcc.h \
    include/google/protobuf/stubs/atomicops_internals_arm_qnx.h \
    include/google/protobuf/stubs/atomicops_internals_atomicword_compat.h \
    include/google/protobuf/stubs/atomicops_internals_generic_c11_atomic.h \
    include/google/protobuf/stubs/atomicops_internals_generic_gcc.h \
    include/google/protobuf/stubs/atomicops_internals_mips_gcc.h \
    include/google/protobuf/stubs/atomicops_internals_power.h \
    include/google/protobuf/stubs/atomicops_internals_ppc_gcc.h \
    include/google/protobuf/stubs/atomicops_internals_solaris.h \
    include/google/protobuf/stubs/atomicops_internals_tsan.h \
    include/google/protobuf/stubs/atomicops_internals_x86_gcc.h \
    include/google/protobuf/stubs/atomicops_internals_x86_msvc.h \
    include/google/protobuf/stubs/bytestream.h \
    include/google/protobuf/stubs/callback.h \
    include/google/protobuf/stubs/casts.h \
    include/google/protobuf/stubs/common.h \
    include/google/protobuf/stubs/fastmem.h \
    include/google/protobuf/stubs/hash.h \
    include/google/protobuf/stubs/logging.h \
    include/google/protobuf/stubs/macros.h \
    include/google/protobuf/stubs/mutex.h \
    include/google/protobuf/stubs/once.h \
    include/google/protobuf/stubs/platform_macros.h \
    include/google/protobuf/stubs/port.h \
    include/google/protobuf/stubs/scoped_ptr.h \
    include/google/protobuf/stubs/shared_ptr.h \
    include/google/protobuf/stubs/singleton.h \
    include/google/protobuf/stubs/status.h \
    include/google/protobuf/stubs/stl_util.h \
    include/google/protobuf/stubs/stringpiece.h \
    include/google/protobuf/stubs/template_util.h \
    include/google/protobuf/stubs/type_traits.h \
    include/google/protobuf/util/delimited_message_util.h \
    include/google/protobuf/util/field_comparator.h \
    include/google/protobuf/util/field_mask_util.h \
    include/google/protobuf/util/json_util.h \
    include/google/protobuf/util/message_differencer.h \
    include/google/protobuf/util/time_util.h \
    include/google/protobuf/util/type_resolver.h \
    include/google/protobuf/util/type_resolver_util.h \
    include/google/protobuf/any.h \
    include/google/protobuf/any.pb.h \
    include/google/protobuf/api.pb.h \
    include/google/protobuf/arena.h \
    include/google/protobuf/arenastring.h \
    include/google/protobuf/descriptor.h \
    include/google/protobuf/descriptor.pb.h \
    include/google/protobuf/descriptor_database.h \
    include/google/protobuf/duration.pb.h \
    include/google/protobuf/dynamic_message.h \
    include/google/protobuf/empty.pb.h \
    include/google/protobuf/extension_set.h \
    include/google/protobuf/field_mask.pb.h \
    include/google/protobuf/generated_enum_reflection.h \
    include/google/protobuf/generated_enum_util.h \
    include/google/protobuf/generated_message_reflection.h \
    include/google/protobuf/generated_message_table_driven.h \
    include/google/protobuf/generated_message_util.h \
    include/google/protobuf/has_bits.h \
    include/google/protobuf/map.h \
    include/google/protobuf/map_entry.h \
    include/google/protobuf/map_entry_lite.h \
    include/google/protobuf/map_field.h \
    include/google/protobuf/map_field_inl.h \
    include/google/protobuf/map_field_lite.h \
    include/google/protobuf/map_type_handler.h \
    include/google/protobuf/message.h \
    include/google/protobuf/message_lite.h \
    include/google/protobuf/metadata.h \
    include/google/protobuf/metadata_lite.h \
    include/google/protobuf/reflection.h \
    include/google/protobuf/reflection_ops.h \
    include/google/protobuf/repeated_field.h \
    include/google/protobuf/service.h \
    include/google/protobuf/source_context.pb.h \
    include/google/protobuf/struct.pb.h \
    include/google/protobuf/text_format.h \
    include/google/protobuf/timestamp.pb.h \
    include/google/protobuf/type.pb.h \
    include/google/protobuf/unknown_field_set.h \
    include/google/protobuf/wire_format.h \
    include/google/protobuf/wire_format_lite.h \
    include/google/protobuf/wire_format_lite_inl.h \
    include/google/protobuf/wrappers.pb.h \
    json11/json11.hpp \
    platforms/huajiao/protocol/ChatRoom.pb.h \
    platforms/huajiao/protocol/CommunicationData.pb.h \
    platforms/huajiao/huajiao.hpp \
    utils/gzip.hpp \
    utils/md5.hpp \
    utils/others.hpp \
    utils/rc4.hpp \
    platforms/platform_base.hpp \
    platforms/platforms.hpp \
    platforms/huajiao/huajiao_config.hpp \
    utils/rw_lock.hpp \
    network/dmp_cs.hpp \
    utils/log.hpp \
    platforms/YY/YY.hpp \
    platforms/douyu/douyu.hpp \
    platforms/douyu/stt.hpp \
    platforms/douyu/douyu_config.hpp \
    platforms/bilibili/bilibili.hpp \
    utils/base64.hpp

SOURCES += \
    json11/json11.cpp \
    platforms/huajiao/protocol/ChatRoom.pb.cc \
    platforms/huajiao/protocol/CommunicationData.pb.cc \
    platforms/huajiao/huajiao.cpp \
    utils/gzip.cpp \
    utils/md5.cpp \
    utils/others.cpp \
    utils/rc4.cpp \
    main.cpp \
    platforms/platform_base.cpp \
    platforms/huajiao/huajiao_config.cpp \
    platforms/platforms.cpp \
    utils/rw_lock.cpp \
    network/dmp_cs.cpp \
    utils/log.cpp \
    platforms/YY/YY.cpp \
    platforms/douyu/douyu.cpp \
    platforms/douyu/stt.cpp \
    platforms/bilibili/bilibili.cpp \
    utils/base64.cpp

