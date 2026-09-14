# Map JSON data

# Inputs
MAPS_DIR = $(DATA_ASM_SUBDIR)/maps
LAYOUTS_DIR = $(DATA_ASM_SUBDIR)/layouts

# Outputs
MAPS_OUTDIR := $(MAPS_DIR)
LAYOUTS_OUTDIR := $(LAYOUTS_DIR)
INCLUDECONSTS_OUTDIR := include/constants

AUTO_GEN_TARGETS += $(INCLUDECONSTS_OUTDIR)/map_groups.h
AUTO_GEN_TARGETS += $(INCLUDECONSTS_OUTDIR)/layouts.h
AUTO_GEN_TARGETS += $(INCLUDECONSTS_OUTDIR)/map_event_ids.h
AUTO_GEN_TARGETS += $(DATA_SRC_SUBDIR)/map_group_count.h

MAP_DIRS := $(dir $(wildcard $(MAPS_DIR)/*/map.json))
MAP_CONNECTIONS := $(patsubst $(MAPS_DIR)/%/,$(MAPS_DIR)/%/connections.inc,$(MAP_DIRS))
MAP_EVENTS := $(patsubst $(MAPS_DIR)/%/,$(MAPS_DIR)/%/events.inc,$(MAP_DIRS))
MAP_HEADERS := $(patsubst $(MAPS_DIR)/%/,$(MAPS_DIR)/%/header.inc,$(MAP_DIRS))
MAP_JSONS := $(patsubst $(MAPS_DIR)/%/,$(MAPS_DIR)/%/map.json,$(MAP_DIRS))

# Poryscript map scripts. scaninc cannot see a .include whose target does not
# exist yet, so event_scripts.o has to depend on these explicitly or Poryscript
# never runs. See D34.
MAP_SCRIPTS := $(patsubst %.pory,%.inc,$(wildcard $(MAPS_DIR)/*/scripts.pory))

$(DATA_ASM_BUILDDIR)/event_scripts.o: $(MAP_SCRIPTS)

$(DATA_ASM_BUILDDIR)/maps.o: $(DATA_ASM_SUBDIR)/maps.s $(LAYOUTS_DIR)/layouts.inc $(LAYOUTS_DIR)/layouts_table.inc $(MAPS_DIR)/headers.inc $(MAPS_DIR)/groups.inc $(MAPS_DIR)/connections.inc $(MAP_CONNECTIONS) $(MAP_HEADERS)
	$(PREPROC) -s $< charmap.txt | $(CPP) $(CPPFLAGS) -I include - | $(PREPROC) -ie $< charmap.txt | $(AS) $(ASFLAGS) -o $@
$(DATA_ASM_BUILDDIR)/map_events.o: $(DATA_ASM_SUBDIR)/map_events.s $(MAPS_DIR)/events.inc $(MAP_EVENTS)
	$(PREPROC) -s $< charmap.txt | $(CPP) $(CPPFLAGS) -I include - | $(PREPROC) -ie $< charmap.txt | $(AS) $(ASFLAGS) -o $@

$(MAPS_OUTDIR)/%/header.inc $(MAPS_OUTDIR)/%/events.inc $(MAPS_OUTDIR)/%/connections.inc: $(MAPS_DIR)/%/map.json $(INCLUDECONSTS_OUTDIR)/map_groups.h $(MAPJSON)
	$(MAPJSON) map emerald $< $(LAYOUTS_DIR)/layouts.json $(@D)


$(MAPS_OUTDIR)/connections.inc $(MAPS_OUTDIR)/groups.inc $(MAPS_OUTDIR)/events.inc $(MAPS_OUTDIR)/headers.inc $(INCLUDECONSTS_OUTDIR)/map_groups.h $(DATA_SRC_SUBDIR)/map_group_count.h: $(MAPS_DIR)/map_groups.json $(MAP_JSONS) .map_version $(MAPJSON)
	@$(MAPJSON) groups $(MAP_VERSION) $(filter %.json,$^) $(MAPS_OUTDIR) $(INCLUDECONSTS_OUTDIR)
	@echo "$(MAPJSON) groups $(MAP_VERSION) $(MAPS_DIR)/map_groups.json <MAP_JSONS> $(MAPS_OUTDIR) $(INCLUDECONSTS_OUTDIR)"

$(LAYOUTS_OUTDIR)/layouts.inc $(LAYOUTS_OUTDIR)/layouts_table.inc $(INCLUDECONSTS_OUTDIR)/layouts.h: $(LAYOUTS_DIR)/layouts.json .map_version $(MAPJSON)
	$(MAPJSON) layouts $(MAP_VERSION) $< $(LAYOUTS_OUTDIR) $(INCLUDECONSTS_OUTDIR)

# Generate constants for map events, which depend on data that's distributed across the map.json files.
# There's a lot of map.json files, so we print an abbreviated output with echo.
$(INCLUDECONSTS_OUTDIR)/map_event_ids.h: $(MAP_JSONS) $(MAPJSON)
	@$(MAPJSON) event_constants emerald $(MAP_JSONS) $(INCLUDECONSTS_OUTDIR)/map_event_ids.h
	@echo "$(MAPJSON) event_constants emerald <MAP_JSONS> $(INCLUDECONSTS_OUTDIR)/map_event_ids.h"

.map_version : FORCE
	@(echo "$(MAP_VERSION)" | cmp $@ -) || echo "$(MAP_VERSION)" > .map_version

FORCE:
.PHONY : FORCE


# ============================================================
# CrystalDust's map rules were REMOVED in Phase 2.
#
# They were vanilla-pokeemerald-era rules that overrode expansion's above and
# invoked mapjson with the old signature -- `mapjson groups emerald <json>`,
# with no output directories -- which expansion's mapjson rejects with a USAGE
# error. The collision was latent: the generated files were already up to date,
# so nothing re-ran until the region_map_sections.json and map.json edits for
# the Johto map section merge forced regeneration.
#
# Same failure mode as CrystalDust's duplicate wild_encounters.h rule (D7).
# See decision D17.
# ============================================================
