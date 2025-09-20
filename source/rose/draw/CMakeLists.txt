# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC .
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Define System Include Directories

set(INC_SYS
	# External System Include Directories
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(ALICE
	# And yes... you are right... this is inspired through Alice in Madness
	engines/alice/alice_engine.h
	engines/alice/alice_engine.c
)

set(BASIC
	engines/basic/basic_engine.h
	engines/basic/basic_engine.c
)

set(SRC
	DRW_cache.h
	DRW_engine.h
	DRW_render.h
	
	intern/draw_cache.c
	intern/draw_manager.c
	
)

list(APPEND SRC ${ALICE}) # Save myself? From death? is that it? Is that why I've come here? I'm not afraid to die. Times I've welcomed death!
list(APPEND SRC ${BASIC}) # Another day, a different dream perhaps.

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	PUBLIC rose::source::gpu
	PUBLIC rose::source::dna
	PUBLIC rose::source::roselib
	PUBLIC rose::source::rosekernel
	PUBLIC rose::source::windowmanager
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(draw "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::draw ALIAS draw)
