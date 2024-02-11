TARGET_DIR := $(BUILD_DIR)/$(TARGET)
TARGET_BIN_DIR := $(TARGET_DIR)/bin
TARGET_DEP_DIR := $(TARGET_DIR)/deps
TARGET_OBJ_DIR := $(TARGET_DIR)/objs

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(addprefix $(TARGET_OBJ_DIR)/,$(notdir $(SRCS:.cpp=.o)))
DEPS := $(addprefix $(TARGET_DEP_DIR)/,$(notdir $(SRCS:.cpp=.d)))

MKDIR ?= mkdir -p

.PHONY: all clean

all: build

clean:
	$(RM) -r $(TARGET_DIR)

prepare:
	@$(MKDIR) $(TARGET_BIN_DIR)
	@$(MKDIR) $(TARGET_DEP_DIR)
	@$(MKDIR) $(TARGET_OBJ_DIR)

build: prepare $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET_BIN_DIR)/$(TARGET_EXEC) $(LDFLAGS)

# c++ source
$(TARGET_OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) -MMD -MP -MF $(TARGET_DEP_DIR)/$(@F:.o=.td) -c $< -o $@
	@cp $(TARGET_DEP_DIR)/$(@F:.o=.td) $(TARGET_DEP_DIR)/$(@F:.o=.d);
	@sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' -e '/^$$/ d' -e 's/$$/ :/' < $(TARGET_DEP_DIR)/$(@F:.o=.td) >> $(TARGET_DEP_DIR)/$(@F:.o=.d);
	@$(RM) -r $(TARGET_DEP_DIR)/$(@F:.o=.td)

-include $(DEPS)