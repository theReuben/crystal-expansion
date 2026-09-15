// CrystalDust's per-tileset day/night palette overrides (D53). Generated from
// data/tilesets/overrides.inc and graphics.inc, which expansion orphaned when it
// moved tileset headers from assembly to C.

static const u16 gTilesetPalOverride_General02[] = INCGFX_U16("data/tilesets/primary/general/palettes/02_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_General07[] = INCGFX_U16("data/tilesets/primary/general/palettes/07_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_NewBark08[] = INCGFX_U16("data/tilesets/secondary/new_bark/palettes/08_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Violet08[] = INCGFX_U16("data/tilesets/secondary/violet/palettes/08_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Violet09[] = INCGFX_U16("data/tilesets/secondary/violet/palettes/09_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Azalea09[] = INCGFX_U16("data/tilesets/secondary/azalea/palettes/09_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Goldenrod08[] = INCGFX_U16("data/tilesets/secondary/goldenrod/palettes/08_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Goldenrod09[] = INCGFX_U16("data/tilesets/secondary/goldenrod/palettes/09_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Goldenrod10[] = INCGFX_U16("data/tilesets/secondary/goldenrod/palettes/10_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Goldenrod12[] = INCGFX_U16("data/tilesets/secondary/goldenrod/palettes/12_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_DeptStore09NoWin[] = INCGFX_U16("data/tilesets/secondary/department_store/palettes/09_over_nowin.pal", ".gbapal");
static const u16 gTilesetPalOverride_DeptStore09Win[] = INCGFX_U16("data/tilesets/secondary/department_store/palettes/09_over_win.pal", ".gbapal");
static const u16 gTilesetPalOverride_EcruteakCity09[] = INCGFX_U16("data/tilesets/secondary/ecruteakcity/palettes/09_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Olivine12[] = INCGFX_U16("data/tilesets/secondary/olivinecity/palettes/12_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Olivine10[] = INCGFX_U16("data/tilesets/secondary/olivinecity/palettes/10_over.pal", ".gbapal");
static const u16 gTilesetPalOverrides_VermilionCity11[] = INCGFX_U16("data/tilesets/secondary/vermilioncity/palettes/11_over.pal", ".gbapal");
static const u16 gTilesetPalOverrides_IndigoPlateau10[] = INCGFX_U16("data/tilesets/secondary/indigoplateau/palettes/10_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Saffron08[] = INCGFX_U16("data/tilesets/secondary/saffroncity/palettes/08_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Saffron11[] = INCGFX_U16("data/tilesets/secondary/saffroncity/palettes/11_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Saffron12[] = INCGFX_U16("data/tilesets/secondary/saffroncity/palettes/12_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Pallet08[] = INCGFX_U16("data/tilesets/secondary/pallettown/palettes/08_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Viridian12[] = INCGFX_U16("data/tilesets/secondary/viridiancity/palettes/12_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Fuchsia08[] = INCGFX_U16("data/tilesets/secondary/fuchsiacity/palettes/08_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Pewter08[] = INCGFX_U16("data/tilesets/secondary/pewtercity/palettes/08_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Pewter11[] = INCGFX_U16("data/tilesets/secondary/pewtercity/palettes/11_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_Lavender11[] = INCGFX_U16("data/tilesets/secondary/lavendertown/palettes/11_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_CeladonMansion07[] = INCGFX_U16("data/tilesets/secondary/celadonmansion/palettes/07_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_DragonsDen_Shrine08[] = INCGFX_U16("data/tilesets/secondary/dragonsden_shrine/palettes/08_over.pal", ".gbapal");
static const u16 gTilesetPalOverride_DragonsDen_Shrine09[] = INCGFX_U16("data/tilesets/secondary/dragonsden_shrine/palettes/09_over.pal", ".gbapal");

static const struct PaletteOverride gTilesetPalOverrides_General[] =
{
    { 2, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_General02 },
    { 7, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_General07 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_NewBark[] =
{
    { 8, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_NewBark08 },
    { 9, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_NewBark08 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_Violet[] =
{
    { 8, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Violet08 },
    { 9, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Violet09 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_Azalea[] =
{
    { 9, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Azalea09 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_Goldenrod[] =
{
    { 8, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Goldenrod08 },
    { 9, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Goldenrod09 },
    { 10, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Goldenrod10 },
    { 12, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Goldenrod12 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_DepartmentStore[] =
{
    { 9, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_DeptStore09Win },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_EcruteakCity[] =
{
    { 9, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_EcruteakCity09 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_OlivineCity[] =
{
    { 12, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Olivine12 },
    { 10, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Olivine10 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_VermilionCity[] =
{
    { 11, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverrides_VermilionCity11 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_IndigoPlateau[] =
{
    { 10, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverrides_IndigoPlateau10 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_SaffronCity[] =
{
    { 8, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Saffron08 },
    { 9, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Goldenrod09 },
    { 11, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Saffron11 },
    { 12, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Saffron12 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_CeladonCity[] =
{
    { 10, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Goldenrod08 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_PalletTown[] =
{
    { 8, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Pallet08 },
    { 9, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Pallet08 },
    { 10, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Pallet08 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_ViridianCity[] =
{
    { 12, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Viridian12 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_FuchsiaCity[] =
{
    { 8, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Fuchsia08 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_PewterCity[] =
{
    { 8, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Pewter08 },
    { 9, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Pewter08 },
    { 11, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Pewter11 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_LavenderTown[] =
{
    { 11, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_Lavender11 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_CeladonMansion[] =
{
    { 7, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_CeladonMansion07 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};

static const struct PaletteOverride gTilesetPalOverrides_DragonsDen_Shrine[] =
{
    { 8, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_DragonsDen_Shrine08 },
    { 9, HOUR_NIGHT, HOUR_MORNING, (void *)gTilesetPalOverride_DragonsDen_Shrine09 },
    { PALOVER_LIST_TERM, 0, 0, NULL },
};
