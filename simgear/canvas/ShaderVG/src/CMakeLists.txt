
if (TARGET SimGearScene)
    target_sources(SimGearScene PRIVATE
        shaders.c
        shArrays.c
        shContext.c
        shExtensions.c
        shGeometry.c
        shImage.c
        shPaint.c
        shParams.c
        shPath.c
        shPipeline.c
        shVectors.c
        shVgu.c
        ../include/vg/openvg.h
        ../include/vg/vgu.h)
endif()
