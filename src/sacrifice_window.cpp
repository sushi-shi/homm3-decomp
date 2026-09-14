// sacrifice_window.cpp - E:\gamedcs\sacrifice_window.cpp (compiland sacrifice_window.obj)
#include <va.h>
#include "creaturetype.h"
#include "sacrifice_window.h"
#include "border.h"
#include "button.h"
#include "game.h"
#include "kb.h"
#include "message.h"
#include "misc.h"
#include "mousemgr.h"
#include "resourcemanager.h"
#include "sample.h"
#include "slider.h"
#include "spellbookwindow.h"
#include "textresource.h"
#include "soundmgr.h"
#include "viewarmywindow.h"
#include "winmgr.h"

static const CreatureType g_deathCreature[145] = {
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_BONE_DRAGON, CREATURE_BONE_DRAGON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_BONE_DRAGON, CREATURE_BONE_DRAGON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_BONE_DRAGON, CREATURE_BONE_DRAGON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_BONE_DRAGON, CREATURE_BONE_DRAGON, CREATURE_BONE_DRAGON,
    CREATURE_BONE_DRAGON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON,
    CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON
};

// Complete adds the nineteenth SoD equipment slot to DC's 18-pair table.
static const long g_slotDefinitions[19][2] = {
    {143, 18}, {202, 230}, {143, 68}, {17, 57}, {196, 172},
    {143, 119}, {65, 57}, {244, 172}, {149, 283}, {17, 131},
    {33, 181}, {49, 232}, {65, 283}, {198, 18}, {244, 18},
    {244, 64}, {244, 110}, {244, 299}, {15, 283}
};

static const long g_rowStart[5][2] = {
    {314, 50}, {314, 120}, {314, 190}, {314, 260}, {395, 330}
};
static const long g_rowSize[5] = {5, 5, 5, 5, 2};

static const long g_constCreatureSources[2][2] = {
    {45, 109}, {128, 305}
};
static const long g_constSourceSizes[2][2] = {
    {3, 2}, {1, 1}
};
static const long g_constCreatureOfferings[2][2] = {
    {334, 109}, {417, 305}
};

#if 0  // @carcass: unlocated Dreamcast bodies and STLport template tail

// E:\gamedcs\sacrifice_window.cpp:125
DC_ONLY(0x123e8c, 0x7C)
void ArtifactOffering::set(const type_artifact* arg, ArtifactSlot slot, const Hero* this_hero)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:170
DC_ONLY(0x123f08, 0x80)
void DollSlotWidget::DollSlotWidget(const DollSlotDefinition* def, long _id)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:178
DC_ONLY(0x123f88, 0x28)
unsigned char DollSlotWidget::handleClick(unsigned char down_click, unsigned char right_click)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:204
DC_ONLY(0x123fb0, 0x78)
void BackpackSlotWidget::BackpackSlotWidget(const IconDefinition* def, long _slot, long _id)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:213
DC_ONLY(0x124028, 0x28)
unsigned char BackpackSlotWidget::handleClick(unsigned char down_click, unsigned char right_click)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:235
DC_ONLY(0x124050, 0x74)
void ArtifactOfferingWidget::ArtifactOfferingWidget(long new_x, long new_y, long new_width, long new_height, long new_item_number, long new_id, const char* image)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:243
DC_ONLY(0x1240c4, 0x28)
unsigned char ArtifactOfferingWidget::handleClick(unsigned char down_click, unsigned char right_click)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:263
DC_ONLY(0x1240ec, 0x80)
void ArmySlotWidget::ArmySlotWidget(long new_x, long new_y, long new_w, long new_h, long new_slot, long new_id, const char* image, unsigned char _left_pane)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:272
DC_ONLY(0x12416c, 0x30)
unsigned char ArmySlotWidget::handleClick(unsigned char down_click, unsigned char right_click)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:287
DC_ONLY(0x12419c, 0x51C)
void SacrificeWindow::SacrificeWindow(Hero* new_hero, int cur_player)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:360
DC_ONLY(0x1246b8, 0x7A8)
void SacrificeWindow::createArtifactWidgets(long* widget_id, int cur_player)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:536
DC_ONLY(0x124e60, 0x76A)
void SacrificeWindow::createCreatureWidgets(long* widget_id, int cur_player)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:699
DC_ONLY(0x1255cc, 0x258)
long SacrificeWindow::createCreatureIcons(long icon_x, long icon_y, long columns, long rows, long item_number, long* widget_id, IconWidget** icon_widgets, IconWidget** selection_widgets, TextWidget** text_widgets, unsigned char left_pane)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:752
DC_ONLY(0x125824, 0x88)
void SacrificeWindow::~SacrificeWindow()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:760
DC_ONLY(0x1258ac, 0xEC)
std::basic_string<char,std::char_traits<char>,std::allocator<char> convertWithCommas(__$ReturnUdt, long value)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:784
DC_ONLY(0x125998, 0xB2)
void SacrificeWindow::updateExperience()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:800
DC_ONLY(0x125a4c, 0x5E)
void updateArtifactWidget(IconWidget* slot_widget, type_artifact artifact)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:821
DC_ONLY(0x125aac, 0x90)
void updateOffering(IconWidget* artifact_widget, TextWidget* value_widget, const ArtifactOffering* offering)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:841
DC_ONLY(0x125b3c, 0xF6)
void SacrificeWindow::updateSlot(ArtifactSlot slot)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:865
DC_ONLY(0x125c34, 0x2A)
void SacrificeWindow::updateAllSlots()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:880
DC_ONLY(0x125c60, 0x17E)
void SacrificeWindow::setArtifactMode()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:914
DC_ONLY(0x125de0, 0x28)
long sacrificeValue(CreatureType creature)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:924
DC_ONLY(0x125e08, 0x25A)
void SacrificeWindow::updateCreatureOffering(CreatureOffering* creature)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:990
DC_ONLY(0x126064, 0x180)
void SacrificeWindow::setCreatureMode()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1036
DC_ONLY(0x1261e4, 0x70)
void SacrificeWindow::pickUpArtifact(type_artifact artifact, ArtifactSlot slot, unsigned char new_artifact)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1053
DC_ONLY(0x126254, 0x90)
void SacrificeWindow::putDownArtifact(unsigned char change_experience)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1071
DC_ONLY(0x1262e4, 0x13A)
void SacrificeWindow::artifactClick(ArtifactSlot slot, unsigned char right_click)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1127
DC_ONLY(0x126420, 0xBA)
void SacrificeWindow::updateBackpack()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1150
DC_ONLY(0x1264dc, 0xDC)
void SacrificeWindow::backpackClick(long slot, unsigned char right_click)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1189
DC_ONLY(0x1265b8, 0x88)
void SacrificeWindow::updateArtifactOffering(long slot)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1200
DC_ONLY(0x126640, 0xDC)
void SacrificeWindow::offeringClick(long slot, unsigned char right_click)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1236
DC_ONLY(0x12671c, 0x90)
int SacrificeWindow::scrollBackpackLeft(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1262
DC_ONLY(0x1267ac, 0x6E)
int SacrificeWindow::scrollBackpackRight(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1289
DC_ONLY(0x12681c, 0x88)
unsigned char SacrificeWindow::addArtifact(type_artifact artifact, ArtifactSlot source)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1310
DC_ONLY(0x1268a4, 0xA0)
void SacrificeWindow::emptyBackpack()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1338
DC_ONLY(0x126944, 0x68)
int SacrificeWindow::emptyBackpack(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1365
DC_ONLY(0x1269ac, 0xC4)
int SacrificeWindow::allArtifacts(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1407
DC_ONLY(0x126a70, 0x1EC)
int SacrificeWindow::sacrifice(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1472
DC_ONLY(0x126c5c, 0x66)
int SacrificeWindow::sacrificeCreatures(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1499
DC_ONLY(0x126cc4, 0x5A)
void SacrificeWindow::returnArtifact(const ArtifactOffering* artifact)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1521
DC_ONLY(0x126d20, 0x78)
void SacrificeWindow::clear()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1544
DC_ONLY(0x126d98, 0x8A)
int SacrificeWindow::exitClick(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1572
DC_ONLY(0x126e24, 0xB2)
void SacrificeWindow::setCreatureSacrifice(long slot, long new_amount)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1599
DC_ONLY(0x126ed8, 0x54)
long SacrificeWindow::getMaxAmount(long slot)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1626
DC_ONLY(0x126f2c, 0xBE)
int SacrificeWindow::allCreatures(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1663
DC_ONLY(0x126fec, 0x78)
int SacrificeWindow::maxCreatures(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1692
DC_ONLY(0x127064, 0x8C)
int SacrificeWindow::sacrificeArtifacts(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1721
DC_ONLY(0x1270f0, 0x29E)
void SacrificeWindow::creatureClick(long slot, unsigned char right_click, unsigned char left_pane)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1796
DC_ONLY(0x127390, 0x74)
void SacrificeWindow::creatureSliderChange(int state, HeroWindow* parent_window)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1815
// RETAIL_LOCATED(0x005653b0, 0x37): not reconstructed. One carve row ahead of
// handle_widget_hover in the Dreamcast roster's own order, and the body is
// that shape: a byte test at +0x75 selecting between two same-class helpers
// (0x562a20 / 0x563150), then heroWindow::DoModal(fadeIn) tail-duplicated
// into both arms. Blocked only on naming those two helpers - neither has a
// call edge that fixes which roster entry it is.
DC_ONLY(0x127404, 0x38)
void SacrificeWindow::doModal(unsigned char fadeIn)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1830
DC_ONLY(0x12743c, 0x48)
void SacrificeWindow::handleWidgetHover(Widget* current_widget)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1846
// The retail row at 0x565430 between handle_widget_hover and the
// transformer-slot rows is ExitDialog, not this. WindowHandler has no retail
// row of its own in this bracket; ExitDialog is reconstructed below.
DC_ONLY(0x127484, 0x36)
int SacrificeWindow::windowHandler(Message& msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1859
DC_ONLY(0x1274bc, 0x38)
int SacrificeWindow::exitDialog(Message& msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1891
DC_ONLY(0x1274f4, 0xA4)
void TransformerSlot::TransformerSlot(long new_x, long new_y, long new_w, long new_h, long new_group, long new_slot, long new_id, const char* image)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1900
DC_ONLY(0x127598, 0x26)
unsigned char TransformerSlot::handleClick(unsigned char down_click, unsigned char right_click)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2056
DC_ONLY(0x1275c0, 0x448)
void SkeletonWindow::SkeletonWindow(ArmyGroup* new_army)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2129
DC_ONLY(0x127a08, 0x84)
void SkeletonWindow::~SkeletonWindow()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2144
DC_ONLY(0x127a8c, 0x3E)
void SkeletonWindow::unselect()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2157
DC_ONLY(0x127acc, 0x9C)
void SkeletonWindow::updateButtons()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2172
DC_ONLY(0x127b68, 0x2E8)
void SkeletonWindow::update(long group, long index)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2216
DC_ONLY(0x127e50, 0x1F6)
void SkeletonWindow::creatureClick(long side, long slot, unsigned char right_click)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2281
DC_ONLY(0x128048, 0x36)
int SkeletonWindow::windowHandler(Message& msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2293
DC_ONLY(0x128080, 0x16)
int SkeletonWindow::exitDialog(Message& msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2306
DC_ONLY(0x128098, 0x48)
void SkeletonWindow::handleWidgetHover(Widget* current_widget)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2326
DC_ONLY(0x1280e0, 0x1CE)
void SkeletonWindow::createCreatureIcons(long icon_x, long icon_y, long columns, long rows, long group_number, long item_number, long* widget_id, IconWidget** icon_widgets, IconWidget** selection_widgets, TextWidget** text_widgets)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2385
DC_ONLY(0x1282b0, 0x5E)
void moveAllArmies(ArmyGroup* source, ArmyGroup* dest)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2407
DC_ONLY(0x128310, 0x9A)
int SkeletonWindow::allCreatures(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2441
DC_ONLY(0x1283ac, 0xBC)
int SkeletonWindow::exitClick(Message* msg)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2471
DC_ONLY(0x128468, 0x134)
int SkeletonWindow::sacrifice(Message* msg)
{
    // @stub
}

// E:\gamedcs\Widget.h:225
DC_ONLY(0x12859c, 0xC)
void Widget::clear_hover_widget()
{
    // @stub
}

// E:\gamedcs\slider.h:104
DC_ONLY(0x1285a8, 0x8)
int Slider::getMaximum()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:172
DC_ONLY(0x1285b0, 0x34)
void* DollSlotWidget::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:172
DC_ONLY(0x1285e4, 0x18)
void DollSlotWidget::~DollSlotWidget()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:206
DC_ONLY(0x1285fc, 0x34)
void* BackpackSlotWidget::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:206
DC_ONLY(0x128630, 0x18)
void BackpackSlotWidget::~BackpackSlotWidget()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:237
DC_ONLY(0x128648, 0x34)
void* ArtifactOfferingWidget::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:237
DC_ONLY(0x12867c, 0x18)
void ArtifactOfferingWidget::~ArtifactOfferingWidget()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:266
DC_ONLY(0x128694, 0x34)
void* ArmySlotWidget::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:266
DC_ONLY(0x1286c8, 0x18)
void ArmySlotWidget::~ArmySlotWidget()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:352
DC_ONLY(0x1286e0, 0x34)
void* SacrificeWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:352
DC_ONLY(0x128714, 0x1C)
void ArtifactOffering::ArtifactOffering()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1894
DC_ONLY(0x128730, 0x34)
void* TransformerSlot::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:1894
DC_ONLY(0x128764, 0x18)
void TransformerSlot::~TransformerSlot()
{
    // @stub
}

// E:\gamedcs\sacrifice_window.cpp:2123
DC_ONLY(0x12877c, 0x34)
void* SkeletonWindow::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// ..\stlport\stl_string.h:826
DC_ONLY(0x1287b0, 0x4C)
char* std::basic_string<char,std::char_traits<char>,std::allocator<char> >::insert(char* __p, char __c)
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x1287fc, 0xC)
unsigned std::vector<ArtifactOffering,std::allocator<ArtifactOffering> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x128808, 0x20)
ArtifactOffering* std::vector<ArtifactOffering,std::allocator<ArtifactOffering> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x128828, 0x1C)
void std::vector<ArtifactOffering,std::allocator<ArtifactOffering> >::vector<ArtifactOffering,std::allocator<ArtifactOffering> >(const std::allocator<ArtifactOffering>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x128844, 0x28)
void std::vector<ArtifactOffering,std::allocator<ArtifactOffering> >::~vector<ArtifactOffering,std::allocator<ArtifactOffering> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x12886c, 0x3C)
void std::vector<ArtifactOffering,std::allocator<ArtifactOffering> >::push_back(const ArtifactOffering* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x1288a8, 0x4)
void std::allocator<ArtifactOffering>::allocator<ArtifactOffering>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x1288ac, 0x4)
void std::allocator<ArtifactOffering>::~allocator<ArtifactOffering>()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x1288b0, 0x20)
TextWidget** std::vector<TextWidget *,std::allocator<TextWidget *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x1288d0, 0x1C)
void std::vector<TextWidget *,std::allocator<TextWidget *> >::vector<TextWidget *,std::allocator<TextWidget *> >(const std::allocator<TextWidget* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x1288ec, 0x28)
void std::vector<TextWidget *,std::allocator<TextWidget *> >::~vector<TextWidget *,std::allocator<TextWidget *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x128914, 0x3C)
void std::vector<TextWidget *,std::allocator<TextWidget *> >::push_back(TextWidget** __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x128950, 0x4)
void std::allocator<TextWidget *>::allocator<TextWidget *>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x128954, 0x4)
void std::allocator<TextWidget *>::~allocator<TextWidget *>()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x128958, 0xC)
unsigned std::vector<IconWidget *,std::allocator<IconWidget *> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x128964, 0x20)
IconWidget** std::vector<IconWidget *,std::allocator<IconWidget *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x128984, 0x1C)
void std::vector<IconWidget *,std::allocator<IconWidget *> >::vector<IconWidget *,std::allocator<IconWidget *> >(const std::allocator<IconWidget* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x1289a0, 0x28)
void std::vector<IconWidget *,std::allocator<IconWidget *> >::~vector<IconWidget *,std::allocator<IconWidget *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x1289c8, 0x3C)
void std::vector<IconWidget *,std::allocator<IconWidget *> >::push_back(IconWidget** __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x128a04, 0x4)
void std::allocator<IconWidget *>::allocator<IconWidget *>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x128a08, 0x4)
void std::allocator<IconWidget *>::~allocator<IconWidget *>()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x128a0c, 0xC)
unsigned std::vector<Sample *,std::allocator<Sample *> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x128a18, 0x20)
Sample** std::vector<Sample *,std::allocator<Sample *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x128a38, 0x1C)
void std::vector<Sample *,std::allocator<Sample *> >::vector<Sample *,std::allocator<Sample *> >(const std::allocator<Sample* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x128a54, 0x28)
void std::vector<Sample *,std::allocator<Sample *> >::~vector<Sample *,std::allocator<Sample *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x128a7c, 0x3C)
void std::vector<Sample *,std::allocator<Sample *> >::push_back(Sample** __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x128ab8, 0x4)
void std::allocator<Sample *>::allocator<Sample *>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x128abc, 0x4)
void std::allocator<Sample *>::~allocator<Sample *>()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x128ac0, 0x4)
ArtifactOffering* std::vector<ArtifactOffering,std::allocator<ArtifactOffering> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x128ac4, 0x2C)
void std::_Vector_base<ArtifactOffering,std::allocator<ArtifactOffering> >::_Vector_base<ArtifactOffering,std::allocator<ArtifactOffering> >(const std::allocator<ArtifactOffering>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x128af0, 0x30)
void std::_Vector_base<ArtifactOffering,std::allocator<ArtifactOffering> >::~_Vector_base<ArtifactOffering,std::allocator<ArtifactOffering> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x128b20, 0x4)
TextWidget** std::vector<TextWidget *,std::allocator<TextWidget *> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x128b24, 0x2C)
void std::_Vector_base<TextWidget *,std::allocator<TextWidget *> >::_Vector_base<TextWidget *,std::allocator<TextWidget *> >(const std::allocator<TextWidget* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x128b50, 0x30)
void std::_Vector_base<TextWidget *,std::allocator<TextWidget *> >::~_Vector_base<TextWidget *,std::allocator<TextWidget *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x128b80, 0x4)
IconWidget** std::vector<IconWidget *,std::allocator<IconWidget *> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x128b84, 0x2C)
void std::_Vector_base<IconWidget *,std::allocator<IconWidget *> >::_Vector_base<IconWidget *,std::allocator<IconWidget *> >(const std::allocator<IconWidget* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x128bb0, 0x30)
void std::_Vector_base<IconWidget *,std::allocator<IconWidget *> >::~_Vector_base<IconWidget *,std::allocator<IconWidget *> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x128be0, 0x4)
Sample** std::vector<Sample *,std::allocator<Sample *> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x128be4, 0x2C)
void std::_Vector_base<Sample *,std::allocator<Sample *> >::_Vector_base<Sample *,std::allocator<Sample *> >(const std::allocator<Sample* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x128c10, 0x30)
void std::_Vector_base<Sample *,std::allocator<Sample *> >::~_Vector_base<Sample *,std::allocator<Sample *> >()
{
    // @stub
}

// ..\stlport\stl_string.h:469
DC_ONLY(0x128c40, 0x18)
void std::_STL_alloc_proxy<ArtifactOffering *,ArtifactOffering,std::allocator<ArtifactOffering> >::~_STL_alloc_proxy<ArtifactOffering *,ArtifactOffering,std::allocator<ArtifactOffering> >()
{
    // @stub
}

// ..\stlport\stl_string.h:469
DC_ONLY(0x128c58, 0x18)
void std::_STL_alloc_proxy<TextWidget * *,TextWidget *,std::allocator<TextWidget *> >::~_STL_alloc_proxy<TextWidget * *,TextWidget *,std::allocator<TextWidget *> >()
{
    // @stub
}

// ..\stlport\stl_string.h:469
DC_ONLY(0x128c70, 0x18)
void std::_STL_alloc_proxy<IconWidget * *,IconWidget *,std::allocator<IconWidget *> >::~_STL_alloc_proxy<IconWidget * *,IconWidget *,std::allocator<IconWidget *> >()
{
    // @stub
}

// ..\stlport\stl_string.h:469
DC_ONLY(0x128c88, 0x18)
void std::_STL_alloc_proxy<Sample * *,Sample *,std::allocator<Sample *> >::~_STL_alloc_proxy<Sample * *,Sample *,std::allocator<Sample *> >()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x128ca0, 0xC)
void std::_STL_alloc_proxy<ArtifactOffering *,ArtifactOffering,std::allocator<ArtifactOffering> >::_STL_alloc_proxy<ArtifactOffering *,ArtifactOffering,std::allocator<ArtifactOffering> >(const std::allocator<ArtifactOffering>* __a, ArtifactOffering** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x128cac, 0x2C)
void std::_STL_alloc_proxy<ArtifactOffering *,ArtifactOffering,std::allocator<ArtifactOffering> >::deallocate(ArtifactOffering* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x128cd8, 0xC)
void std::_STL_alloc_proxy<TextWidget * *,TextWidget *,std::allocator<TextWidget *> >::_STL_alloc_proxy<TextWidget * *,TextWidget *,std::allocator<TextWidget *> >(const std::allocator<TextWidget* __a, TextWidget*** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x128ce4, 0x2C)
void std::_STL_alloc_proxy<TextWidget * *,TextWidget *,std::allocator<TextWidget *> >::deallocate(TextWidget** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x128d10, 0xC)
void std::_STL_alloc_proxy<IconWidget * *,IconWidget *,std::allocator<IconWidget *> >::_STL_alloc_proxy<IconWidget * *,IconWidget *,std::allocator<IconWidget *> >(const std::allocator<IconWidget* __a, IconWidget*** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x128d1c, 0x2C)
void std::_STL_alloc_proxy<IconWidget * *,IconWidget *,std::allocator<IconWidget *> >::deallocate(IconWidget** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x128d48, 0xC)
void std::_STL_alloc_proxy<Sample * *,Sample *,std::allocator<Sample *> >::_STL_alloc_proxy<Sample * *,Sample *,std::allocator<Sample *> >(const std::allocator<Sample* __a, Sample*** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x128d54, 0x2C)
void std::_STL_alloc_proxy<Sample * *,Sample *,std::allocator<Sample *> >::deallocate(Sample** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x128d80, 0x1C)
void std::allocator<ArtifactOffering>::deallocate(ArtifactOffering* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x128d9c, 0x1C)
void std::allocator<TextWidget *>::deallocate(TextWidget** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x128db8, 0x1C)
void std::allocator<IconWidget *>::deallocate(IconWidget** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x128dd4, 0x1C)
void std::allocator<Sample *>::deallocate(Sample** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_string.c:338
DC_ONLY(0x128df0, 0xF4)
char* std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_M_insert_aux(char* __p, char __c)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x128ee4, 0xD0)
void std::vector<ArtifactOffering,std::allocator<ArtifactOffering> >::_M_insert_overflow(ArtifactOffering* __position, const ArtifactOffering* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x128fb4, 0xCC)
void std::vector<TextWidget *,std::allocator<TextWidget *> >::_M_insert_overflow(TextWidget** __position, TextWidget** __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x129080, 0xCC)
void std::vector<IconWidget *,std::allocator<IconWidget *> >::_M_insert_overflow(IconWidget** __position, IconWidget** __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x12914c, 0xCC)
void std::vector<Sample *,std::allocator<Sample *> >::_M_insert_overflow(Sample** __position, Sample** __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x129218, 0x30)
void std::destroy(ArtifactOffering* __first, ArtifactOffering* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x129248, 0x34)
void std::construct(ArtifactOffering* __p, const ArtifactOffering* __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x12927c, 0x30)
void std::destroy(TextWidget** __first, TextWidget** __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x1292ac, 0x28)
void std::construct(TextWidget** __p, TextWidget** __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x1292d4, 0x30)
void std::destroy(IconWidget** __first, IconWidget** __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x129304, 0x28)
void std::construct(IconWidget** __p, IconWidget** __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x12932c, 0x30)
void std::destroy(Sample** __first, Sample** __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x12935c, 0x28)
void std::construct(Sample** __p, Sample** __value)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x129384, 0x4)
std::allocator<ArtifactOffering>* std::__stl_alloc_rebind(std::allocator<ArtifactOffering>* __a, const ArtifactOffering* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x129388, 0x4)
std::allocator<TextWidget* std::__stl_alloc_rebind(std::allocator<TextWidget* __a, TextWidget** __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x12938c, 0x4)
std::allocator<IconWidget* std::__stl_alloc_rebind(std::allocator<IconWidget* __a, IconWidget** __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x129390, 0x4)
std::allocator<Sample* std::__stl_alloc_rebind(std::allocator<Sample* __a, Sample** __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x129394, 0x28)
ArtifactOffering* std::_STL_alloc_proxy<ArtifactOffering *,ArtifactOffering,std::allocator<ArtifactOffering> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x1293bc, 0xC)
unsigned std::vector<TextWidget *,std::allocator<TextWidget *> >::size()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x1293c8, 0x28)
TextWidget** std::_STL_alloc_proxy<TextWidget * *,TextWidget *,std::allocator<TextWidget *> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x1293f0, 0x28)
IconWidget** std::_STL_alloc_proxy<IconWidget * *,IconWidget *,std::allocator<IconWidget *> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x129418, 0x28)
Sample** std::_STL_alloc_proxy<Sample * *,Sample *,std::allocator<Sample *> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x129440, 0x28)
ArtifactOffering* std::allocator<ArtifactOffering>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x129468, 0x24)
TextWidget** std::allocator<TextWidget *>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x12948c, 0x24)
IconWidget** std::allocator<IconWidget *>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x1294b0, 0x24)
Sample** std::allocator<Sample *>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x1294d4, 0x28)
void std::construct(char* __p, const char* __value)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x1294fc, 0x38)
ArtifactOffering* std::uninitialized_copy(ArtifactOffering* __first, ArtifactOffering* __last, ArtifactOffering* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x129534, 0x38)
ArtifactOffering* std::uninitialized_fill_n(ArtifactOffering* __first, unsigned __n, const ArtifactOffering* __x)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x12956c, 0x38)
TextWidget** std::uninitialized_copy(TextWidget** __first, TextWidget** __last, TextWidget** __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x1295a4, 0x38)
TextWidget** std::uninitialized_fill_n(TextWidget** __first, unsigned __n, TextWidget** __x)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x1295dc, 0x38)
IconWidget** std::uninitialized_copy(IconWidget** __first, IconWidget** __last, IconWidget** __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x129614, 0x38)
IconWidget** std::uninitialized_fill_n(IconWidget** __first, unsigned __n, IconWidget** __x)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x12964c, 0x38)
Sample** std::uninitialized_copy(Sample** __first, Sample** __last, Sample** __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x129684, 0x38)
Sample** std::uninitialized_fill_n(Sample** __first, unsigned __n, Sample** __x)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x1296bc, 0x4)
ArtifactOffering* std::value_type(const ArtifactOffering* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x1296c0, 0x1C)
void std::__destroy(ArtifactOffering* __first, ArtifactOffering* __last, ArtifactOffering* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x1296dc, 0x4)
TextWidget** std::value_type(TextWidget** __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x1296e0, 0x1C)
void std::__destroy(TextWidget** __first, TextWidget** __last, TextWidget** __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x1296fc, 0x4)
IconWidget** std::value_type(IconWidget** __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x129700, 0x1C)
void std::__destroy(IconWidget** __first, IconWidget** __last, IconWidget** __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x12971c, 0x4)
Sample** std::value_type(Sample** __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x129720, 0x1C)
void std::__destroy(Sample** __first, Sample** __last, Sample** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x12973c, 0x1C)
ArtifactOffering* std::__uninitialized_copy(ArtifactOffering* __first, ArtifactOffering* __last, ArtifactOffering* __result, ArtifactOffering* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x129758, 0x1C)
ArtifactOffering* std::__uninitialized_fill_n(ArtifactOffering* __first, unsigned __n, const ArtifactOffering* __x, ArtifactOffering* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x129774, 0x1C)
TextWidget** std::__uninitialized_copy(TextWidget** __first, TextWidget** __last, TextWidget** __result, TextWidget** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x129790, 0x1C)
TextWidget** std::__uninitialized_fill_n(TextWidget** __first, unsigned __n, TextWidget** __x, TextWidget** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x1297ac, 0x1C)
IconWidget** std::__uninitialized_copy(IconWidget** __first, IconWidget** __last, IconWidget** __result, IconWidget** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x1297c8, 0x1C)
IconWidget** std::__uninitialized_fill_n(IconWidget** __first, unsigned __n, IconWidget** __x, IconWidget** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x1297e4, 0x1C)
Sample** std::__uninitialized_copy(Sample** __first, Sample** __last, Sample** __result, Sample** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x129800, 0x1C)
Sample** std::__uninitialized_fill_n(Sample** __first, unsigned __n, Sample** __x, Sample** __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x12981c, 0x30)
void std::__destroy_aux(ArtifactOffering* __first, ArtifactOffering* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x12984c, 0x30)
void std::__destroy_aux(TextWidget** __first, TextWidget** __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x12987c, 0x30)
void std::__destroy_aux(IconWidget** __first, IconWidget** __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x1298ac, 0x30)
void std::__destroy_aux(Sample** __first, Sample** __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x1298dc, 0x3C)
ArtifactOffering* std::__uninitialized_copy_aux(ArtifactOffering* __first, ArtifactOffering* __last, ArtifactOffering* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x129918, 0x3C)
ArtifactOffering* std::__uninitialized_fill_n_aux(ArtifactOffering* __first, unsigned __n, const ArtifactOffering* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x129954, 0x3C)
TextWidget** std::__uninitialized_copy_aux(TextWidget** __first, TextWidget** __last, TextWidget** __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x129990, 0x3C)
TextWidget** std::__uninitialized_fill_n_aux(TextWidget** __first, unsigned __n, TextWidget** __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x1299cc, 0x3C)
IconWidget** std::__uninitialized_copy_aux(IconWidget** __first, IconWidget** __last, IconWidget** __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x129a08, 0x3C)
IconWidget** std::__uninitialized_fill_n_aux(IconWidget** __first, unsigned __n, IconWidget** __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x129a44, 0x3C)
Sample** std::__uninitialized_copy_aux(Sample** __first, Sample** __last, Sample** __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x129a80, 0x3C)
Sample** std::__uninitialized_fill_n_aux(Sample** __first, unsigned __n, Sample** __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x129abc, 0x1C)
void std::destroy(ArtifactOffering* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x129ad8, 0x1C)
void std::destroy(TextWidget** __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x129af4, 0x1C)
void std::destroy(IconWidget** __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x129b10, 0x1C)
void std::destroy(Sample** __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x129b2c, 0x4)
void std::__destroy_aux()
{
    // @stub
}

#endif  // @carcass

VA(0x0055fc30, 0xab)  // dc 0x123e8c
void ArtifactOffering::set(const type_artifact* artifact, long slot,
                                 const Hero* owner)
{
    long artifactClass =
        g_artifactTraits[artifact->m_artifactId].m_artifactClass;
    m_artifactId = artifact->m_artifactId;
    m_extra = artifact->m_extra;
    m_source = slot;
    m_value = 0;

    switch (artifactClass) {
    case SACRIFICE_ARTIFACT_CLASS_TREASURE:
        m_value = 1000;
        break;
    case SACRIFICE_ARTIFACT_CLASS_MINOR:
        m_value = 1500;
        break;
    case SACRIFICE_ARTIFACT_CLASS_MAJOR:
        m_value = 3000;
        break;
    case SACRIFICE_ARTIFACT_CLASS_RELIC:
        m_value = 6000;
        break;
    }
    m_value = static_cast<long>(
        m_value * owner->getExperienceBonusFactor());
}

// E:\gamedcs\sacrifice_window.cpp:170
// All Complete calls are expanded into the artifact widget builder.
inline DollSlotWidget::DollSlotWidget(
    const DollSlotDefinition& def, long id)
    : IconWidget(def.m_x, def.m_y, def.m_width, def.m_height, id, def.m_image,
                 0, 0, 0, 0, 16)
{
    m_slot = def.m_slot;
}

// E:\gamedcs\sacrifice_window.cpp:204
// All Complete calls are expanded into the artifact widget builder.
inline BackpackSlotWidget::BackpackSlotWidget(
    const IconDefinition& def, long newSlot, long id)
    : IconWidget(def.m_x, def.m_y, def.m_width, def.m_height, id, def.m_image,
                 0, 0, 0, 0, 16)
{
    m_slot = newSlot;
}

// E:\gamedcs\sacrifice_window.cpp:235
// All Complete calls are expanded into the artifact widget builder.
inline ArtifactOfferingWidget::ArtifactOfferingWidget(
    long x, long y, long width, long height, long newItemNumber,
    long id, const char* image)
    : IconWidget(x, y, width, height, id, image, 0, 0, 0, 0, 16)
{
    m_itemNumber = newItemNumber;
}

// E:\gamedcs\sacrifice_window.cpp:178
// The doll-slot twin of the two below: one carve row ahead of them, in the
// Dreamcast roster's own order (doll, backpack, offering, army), same 0x26
// body shape, and forwarding to artifact_click - the equipped-slot handler.
// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x0055fce0, 0x26)  // linkorder + iconWidget parent/+0x48 read, dc 0x123f88
bool DollSlotWidget::handleClick(
    bool downClick, bool rightClick)
{
    if (downClick) {
        static_cast<SacrificeWindow*>(m_parentWindow)->artifactClick(
            m_slot, rightClick);
        return 1;
    }
    return 0;
}

// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x0055fd10, 0x26)
bool BackpackSlotWidget::handleClick(
    bool downClick, bool rightClick)
{
    if (downClick) {
        static_cast<SacrificeWindow*>(m_parentWindow)->backpackClick(
            m_slot, rightClick);
        return 1;
    }
    return 0;
}

// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x0055fd40, 0x26)
bool ArtifactOfferingWidget::handleClick(
    bool downClick, bool rightClick)
{
    if (downClick) {
        static_cast<SacrificeWindow*>(m_parentWindow)->offeringClick(
            m_itemNumber, rightClick);
        return 1;
    }
    return 0;
}

// Vtable 0x64165c slot 0 is the last of four slot-widget address-takers for
// this ICF representative. Its call target is the 0x5654b0 representative
// immediately before type_transformer_slot::handle_click, fixing the
// canonical source identity despite the three earlier folded aliases.
// E:\gamedcs\sacrifice_window.cpp:1894
VA_COMPGEN(0x0055fd70, 0x21, SCALAR_DELETING_DTOR,
           TransformerSlot)

// E:\gamedcs\sacrifice_window.cpp:272
// The army-slot member of the same family, 0x2a rather than 0x26 because it
// forwards a THIRD value: the byte at +0x4c alongside the dword at +0x48.
// That is creature_click's (slot, right_click, left_pane) exactly, and the
// pair matches the Dreamcast constructor's (new_slot, _left_pane).
// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x0055fda0, 0x2a)  // linkorder + the +0x48/+0x4c pair, dc 0x12416c
bool ArmySlotWidget::handleClick(
    bool downClick, bool rightClick)
{
    if (downClick) {
        static_cast<SacrificeWindow*>(m_parentWindow)->creatureClick(
            m_slot, rightClick, m_leftPane);
        return 1;
    }
    return 0;
}

// E:\gamedcs\sacrifice_window.cpp:263
// Complete expands both calls in create_creature_icons and retains no
// separately claimable copy. The base constructor arguments and the three
// trailing stores are byte-proven by those two expansions.
ArmySlotWidget::ArmySlotWidget(
    long newX, long newY, long newW, long newH, long newSlot,
    long newId, const char* image, unsigned char newLeftPane)
    : IconWidget(newX, newY, newW, newH, newId, image,
                 0, 0, 0, 0, 16)
{
    m_slot = newSlot;
    m_leftPane = newLeftPane;
}

VA(0x0055fdd0, 0x574)  // dc 0x12419c
SacrificeWindow::SacrificeWindow(Hero* newHero, int curPlayer)
    : CAdvPopup(0, 0, 800, 600, 0)
{
    m_currentHero = newHero;
    m_x = 100;
    m_y = 2;
    m_width = 600;
    m_height = 593;
    m_type = 18;

    long widgetId = 100;
    Widget* newWidget;
    m_widgets.reserve(150);
    createArtifactWidgets(widgetId, curPlayer);
    createCreatureWidgets(widgetId, curPlayer);

    m_widgets.push_back(new TextWidget(
        24, 414, 104, 50,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_NEXT_LEVEL),
        "smalfont.fnt", Font::HEADING, widgetId++, 1, 0, 8));

    m_experienceWidget = new TextWidget(
        44, 468, 66, 16, g_emptyRolloverText, "smalfont.fnt",
        Font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_experienceWidget);

    m_widgets.push_back(new TextWidget(
        24, 492, 104, 42,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_TOTAL_EXPERIENCE),
        "smalfont.fnt", Font::HEADING, widgetId++, 1, 0, 8));

    m_experienceTotalWidget = new TextWidget(
        41, 536, 66, 16, g_emptyRolloverText, "smalfont.fnt",
        Font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_experienceTotalWidget);

    m_sacrificeButton = new FuncButton(
        269, 520, 64, 32, widgetId++, "AltSacr.def",
        sacrifice, 0, 1);
    m_widgets.push_back(m_sacrificeButton);

    newWidget = new FuncButton(
        515, 520, 64, 30, widgetId++, "iOkay.def",
        exitClick, 0, 1);
    static_cast<FuncButton*>(newWidget)->setHotkey(28);
    newWidget->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_EXIT_BUTTON].m_text, 0, 1);
    m_widgets.push_back(newWidget);

    m_rolloverText = new TextWidget(
        8, 567, 584, 18, g_emptyRolloverText, "smalfont.fnt",
        Font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_rolloverText);

    int townType = g_heroClasses[m_currentHero->m_heroClass].m_townType;
    m_canSacrificeArtifacts =
        !(townType > TOWN_TOWER && townType < TOWN_STRONGHOLD);
    m_canSacrificeCreatures = townType > TOWN_TOWER;
    m_totalExperience = 0;

    for (Widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

// E:\gamedcs\sacrifice_window.cpp:360
VA(0x00560380, 0xD67)  // ctor caller + dc name/order/locals, dc 0x1246b8
void SacrificeWindow::createArtifactWidgets(
    long& widgetId, int curPlayer)
{
    m_artifactWidgets.reserve(80);

    BitmapBorder* background = new BitmapBorder(
        0, 0, 600, 593, widgetId++,
        g_game->m_f1f698 >= 2 ? "AltrArt2.pcx" : "AltarArt.pcx", 0x800);
    background->setPlayerPaletteColors(curPlayer);
    m_widgets.push_back(background);
    m_artifactWidgets.push_back(background);

    DollSlotDefinition def;
    IconWidget* currentIconWidget;
    def.m_width = 44;
    def.m_height = 44;
    def.m_image = "artifact.def";
    long count = g_game->m_f1f698 >= 2 ? 19 : 18;
    long i;
    for (i = 0; i < count; ++i) {
        def.m_x = g_slotDefinitions[i][0];
        def.m_y = g_slotDefinitions[i][1];
        def.m_slot = i;

        currentIconWidget = new IconWidget(
            def.m_x, def.m_y, def.m_width, def.m_height, widgetId++, def.m_image,
            0, 0, 0, 0, 16);
        m_widgets.push_back(currentIconWidget);
        m_slotBackWidgets.push_back(currentIconWidget);

        currentIconWidget = new DollSlotWidget(def, widgetId++);
        m_widgets.push_back(currentIconWidget);
        m_slotWidgets.push_back(currentIconWidget);
        m_artifactWidgets.push_back(currentIconWidget);
    }

    def.m_x = 43;
    def.m_y = 352;
    for (i = 0; i < 5; ++i) {
        currentIconWidget = new BackpackSlotWidget(def, i, widgetId++);
        def.m_x += 44;
        m_widgets.push_back(currentIconWidget);
        m_backpackWidgets.push_back(currentIconWidget);
        m_artifactWidgets.push_back(currentIconWidget);
    }

    m_leftBackpackButton = new FuncButton(
        20, 352, 22, 46, widgetId++, "hsbtns3.def",
        scrollBackpackLeft, 0, 1);
    m_widgets.push_back(m_leftBackpackButton);
    m_artifactWidgets.push_back(m_leftBackpackButton);

    m_rightBackpackButton = new FuncButton(
        264, 352, 22, 46, widgetId++, "hsbtns5.def",
        scrollBackpackRight, 0, 1);
    m_widgets.push_back(m_rightBackpackButton);
    m_artifactWidgets.push_back(m_rightBackpackButton);

    // Original local: artifact_offering; DC line 446 invokes the generated
    // default constructor, whose base call supplies TArtifact(-1).
    ArtifactOffering artifactOffering;
    long itemCount = 0;
    TextWidget* currentTextWidget;
    for (long j = 0; j < 5; ++j) {
        long itemX = g_rowStart[j][0];
        int itemY = g_rowStart[j][1];
        long textX = itemX - 2;
        long textY = itemY + 47;
        for (count = g_rowSize[j]; count > 0; --count) {
            currentTextWidget = new TextWidget(
                textX, textY, 48, 16, g_emptyRolloverText,
                "smalfont.fnt", Font::PRIMARY, widgetId++, 1, 0, 8);
            m_widgets.push_back(currentTextWidget);
            m_artifactWidgets.push_back(currentTextWidget);
            m_artifactValueWidgets.push_back(currentTextWidget);

            currentIconWidget = new ArtifactOfferingWidget(
                itemX, itemY, 44, 44, itemCount++,
                widgetId++, "artifact.def");
            m_widgets.push_back(currentIconWidget);
            m_artifactWidgets.push_back(currentIconWidget);
            m_artifactOfferingWidgets.push_back(currentIconWidget);
            m_artifactOfferings.push_back(artifactOffering);

            textX += 54;
            itemX += 54;
        }
    }

    m_currentArtifactValue = new TextWidget(
        269, 492, 66, 16,
        DATA_COMPGEN(0x00682a08, artifactZeroText, "0"),
        "smalfont.fnt", Font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_currentArtifactValue);
    m_artifactWidgets.push_back(m_currentArtifactValue);

    m_currentArtifactWidget = new IconWidget(
        279, 440, 44, 44, widgetId++, "artifact.def",
        0, 0, 0, 0, 16);
    m_currentArtifactWidget->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CURRENT_ARTIFACT].m_text, 0, 1);
    m_widgets.push_back(m_currentArtifactWidget);
    m_artifactWidgets.push_back(m_currentArtifactWidget);

    m_emptyBackpackButton = new FuncButton(
        146, 520, 64, 32, widgetId++, "AltEmBk.def",
        emptyBackpack, 0, 1);
    m_emptyBackpackButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_EMPTY_BACKPACK].m_text, 0, 1);
    m_widgets.push_back(m_emptyBackpackButton);
    m_artifactWidgets.push_back(m_emptyBackpackButton);

    m_allArtifactsButton = new FuncButton(
        392, 520, 64, 32, widgetId++, "AltFill.def",
        allArtifacts, 0, 1);
    m_allArtifactsButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_ALL_ARTIFACTS].m_text, 0, 1);
    m_widgets.push_back(m_allArtifactsButton);
    m_artifactWidgets.push_back(m_allArtifactsButton);

    m_creaturesButton = new FuncButton(
        515, 421, 64, 32, widgetId++, "AltSacC.def",
        sacrificeCreatures, 0, 1);
    m_creaturesButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_SACRIFICE_CREATURES_BUTTON].m_text,
        0, 1);
    m_widgets.push_back(m_creaturesButton);
    m_artifactWidgets.push_back(m_creaturesButton);

    currentTextWidget = new TextWidget(
        317, 23, 256, 18,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_ARTIFACTS_TITLE),
        "smalfont.fnt", Font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(currentTextWidget);
    m_artifactWidgets.push_back(currentTextWidget);

    currentTextWidget = new TextWidget(
        159, 415, 283, 18,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_CREATURES_TITLE),
        "smalfont.fnt", Font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(currentTextWidget);
    m_artifactWidgets.push_back(currentTextWidget);
}

VA(0x005610f0, 0xE73)  // dc 0x124e60
void SacrificeWindow::createCreatureWidgets(
    long& widgetId, int curPlayer)
{
    std::string buffer;
    long count;
    m_creatureWidgets.reserve(60);

    BitmapBorder* background = new BitmapBorder(
        0, 0, 600, 593, widgetId++, "AltarMon.pcx", 0x800);
    background->setPlayerPaletteColors(curPlayer);
    m_widgets.push_back(background);
    m_creatureWidgets.push_back(background);

    buffer = formatString(
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_HERO_NAME),
        m_currentHero->m_name);

    TextWidget* currentTextWidget = new TextWidget(
        28, 21, 256, 18, buffer.c_str(), "smalfont.fnt",
        Font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(currentTextWidget);
    m_creatureWidgets.push_back(currentTextWidget);

    m_creatureNameWidget = new TextWidget(
        29, 56, 256, 42, g_emptyRolloverText, "medfont.fnt",
        Font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_creatureNameWidget);
    m_creatureWidgets.push_back(m_creatureNameWidget);

    currentTextWidget = new TextWidget(
        317, 21, 256, 18,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_SOURCE_CREATURES),
        "smalfont.fnt", Font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(currentTextWidget);
    m_creatureWidgets.push_back(currentTextWidget);

    currentTextWidget = new TextWidget(
        318, 56, 256, 42,
        g_generalText->getText(SACRIFICE_GENERAL_TEXT_OFFERED_CREATURES),
        "smalfont.fnt", Font::HEADING, widgetId++, 1, 0, 8);
    m_widgets.push_back(currentTextWidget);
    m_creatureWidgets.push_back(currentTextWidget);

    IconWidget* newIconWidgets[7];
    IconWidget* selectionFrames[7];
    TextWidget* newTextWidgets[7];
    long itemNumber = 0;

    for (long defIndex = 0; defIndex < 2; ++defIndex) {
        count = createCreatureIcons(
            g_constCreatureSources[defIndex][0],
            g_constCreatureSources[defIndex][1],
            g_constSourceSizes[defIndex][0],
            g_constSourceSizes[defIndex][1], itemNumber, widgetId,
            newIconWidgets, selectionFrames, newTextWidgets, 1);
        for (long i = 0; i < count; ++i) {
            m_creatureOfferings[itemNumber + i].m_iconWidget =
                newIconWidgets[i];
            m_creatureOfferings[itemNumber + i].m_creatureCountText =
                newTextWidgets[i];
            m_creatureOfferings[itemNumber + i].m_sourceSelectionFrame =
                selectionFrames[i];
        }
        itemNumber += count;
    }

    itemNumber = 0;
    for (defIndex = 0; defIndex < 2; ++defIndex) {
        count = createCreatureIcons(
            g_constCreatureOfferings[defIndex][0],
            g_constCreatureOfferings[defIndex][1],
            g_constSourceSizes[defIndex][0],
            g_constSourceSizes[defIndex][1], itemNumber, widgetId,
            newIconWidgets, selectionFrames, newTextWidgets, 0);
        for (long i = 0; i < count; ++i) {
            CreatureOffering& offering =
                m_creatureOfferings[itemNumber + i];
            offering.m_selectionWidget = newIconWidgets[i];
            offering.m_experienceText = newTextWidgets[i];
            offering.m_offeringSelectionFrame = selectionFrames[i];
        }
        itemNumber += count;
    }

    m_currentCreature.m_creatureCountText = new TextWidget(
        145, 493, 66, 16, g_emptyRolloverText, "smalfont.fnt",
        Font::PRIMARY, widgetId++, 1, 0, 8);
    m_currentCreature.m_creatureCountText->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CURRENT_CREATURE_AMOUNT].m_text,
        0, 1);
    m_widgets.push_back(m_currentCreature.m_creatureCountText);
    m_creatureWidgets.push_back(m_currentCreature.m_creatureCountText);

    m_currentCreature.m_iconWidget = new ArmySlotWidget(
        149, 421, 58, 64, -1, widgetId++, "Twcrport.def", 1);
    m_currentCreature.m_iconWidget->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CURRENT_SOURCE_CREATURE].m_text,
        0, 1);
    m_widgets.push_back(m_currentCreature.m_iconWidget);
    m_creatureWidgets.push_back(m_currentCreature.m_iconWidget);
    m_currentCreature.m_sourceSelectionFrame = 0;

    m_currentCreature.m_experienceText = new TextWidget(
        391, 493, 66, 16, g_emptyRolloverText, "smalfont.fnt",
        Font::PRIMARY, widgetId++, 1, 0, 8);
    m_currentCreature.m_experienceText->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CURRENT_OFFERING_AMOUNT].m_text,
        0, 1);
    m_widgets.push_back(m_currentCreature.m_experienceText);
    m_creatureWidgets.push_back(m_currentCreature.m_experienceText);

    m_currentCreature.m_selectionWidget = new ArmySlotWidget(
        395, 421, 58, 64, -2, widgetId++, "TwCrPort.def", 0);
    m_currentCreature.m_selectionWidget->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CURRENT_OFFERING_CREATURE].m_text,
        0, 1);
    m_widgets.push_back(m_currentCreature.m_selectionWidget);
    m_creatureWidgets.push_back(m_currentCreature.m_selectionWidget);
    m_currentCreature.m_offeringSelectionFrame = 0;

    m_creatureSlider = new Slider(
        230, 479, 138, 16, widgetId++, 1, creatureSliderChange,
        Slider::BROWN, 0, 0);
    m_creatureSlider->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_CREATURE_SLIDER].m_text, 0, 1);
    m_widgets.push_back(m_creatureSlider);
    m_creatureWidgets.push_back(m_creatureSlider);

    m_maxCreaturesButton = new FuncButton(
        146, 520, 64, 32, widgetId++, "IrcBtns.def",
        maxCreatures, 0, 1);
    m_maxCreaturesButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_MAX_CREATURES].m_text, 0, 1);
    m_widgets.push_back(m_maxCreaturesButton);
    m_creatureWidgets.push_back(m_maxCreaturesButton);

    m_allCreaturesButton = new FuncButton(
        392, 520, 64, 32, widgetId++, "AltArmy.def",
        allCreatures, 0, 1);
    m_allCreaturesButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_ALL_CREATURES].m_text, 0, 1);
    m_widgets.push_back(m_allCreaturesButton);
    m_creatureWidgets.push_back(m_allCreaturesButton);

    m_artifactsButton = new FuncButton(
        515, 421, 54, 32, widgetId++, "AltArt.def",
        sacrificeArtifacts, 0, 1);
    m_artifactsButton->setHelpText(
        g_sacrificeWindowHelp[
            SACRIFICE_HELP_SACRIFICE_ARTIFACTS_BUTTON].m_text,
        0, 1);
    m_widgets.push_back(m_artifactsButton);
    m_creatureWidgets.push_back(m_artifactsButton);
}

VA(0x00561f70, 0x427)  // dc 0x1255cc
long SacrificeWindow::createCreatureIcons(
    long iconX, long iconY, long columns, long rows, long itemNumber,
    long& widgetId, IconWidget** iconWidgets,
    IconWidget** selectionWidgets, TextWidget** textWidgets,
    unsigned char leftPane)
{
    long count = 0;
    long textX = iconX - 4;
    long textY = iconY + 68;

    for (long row = 0; row < rows; ++row) {
        for (long column = 0; column < columns; ++column) {
            textWidgets[count] = new TextWidget(
                textX, textY, 66, 16, g_emptyRolloverText,
                "smalfont.fnt", Font::PRIMARY, widgetId++, 1, 0, 8);
            textWidgets[count]->setHelpText(
                g_sacrificeWindowHelp[SACRIFICE_HELP_CREATURE_SLOT].m_text,
                0, 1);
            m_widgets.push_back(textWidgets[count]);
            m_creatureWidgets.push_back(textWidgets[count]);

            iconWidgets[count] = new ArmySlotWidget(
                iconX, iconY, 58, 64, itemNumber + count, widgetId++,
                "twcrport.def", leftPane);
            m_widgets.push_back(iconWidgets[count]);
            m_creatureWidgets.push_back(iconWidgets[count]);

            selectionWidgets[count] = new ArmySlotWidget(
                iconX, iconY, 58, 64, itemNumber + count, widgetId++,
                "TwCrPort.def", leftPane);
            m_widgets.push_back(selectionWidgets[count]);
            m_creatureWidgets.push_back(selectionWidgets[count]);
            selectionWidgets[count]->setIconFrame(1);

            ++count;
            textX += 83;
            iconX += 83;
        }
        textX -= columns * 83;
        iconX -= columns * 83;
        textY += 98;
        iconY += 98;
    }
    return count;
}

// Vtable 0x641620 slot 0 is the canonical VC6 deleting wrapper for the
// destructor below.
VA_COMPGEN(0x00560350, 0x21, SCALAR_DELETING_DTOR,
           SacrificeWindow)

VA(0x005623a0, 0x15b)  // dc 0x125824
SacrificeWindow::~SacrificeWindow()
{
    deleteWidgets();
}

std::string convertWithCommas(long value)
{
    long digits = 0;
    std::string result;
    result = formatString(
        DATA_COMPGEN(0x00660a1c, decimalFormat, "%d"), value);

    long position = result.length();
    while (position--) {
        if (++digits == COMMA_DIGIT_THRESHOLD) {
            result.insert(result.begin() + position + 1, ',');
            digits = 1;
        }
    }
    return result;
}

VA(0x00562500, 0x15a)  // dc 0x125998
void SacrificeWindow::updateExperience()
{
    std::string text;
    text = convertWithCommas(
        Hero::getExperience(m_currentHero->m_level + 1)
        - m_currentHero->m_experience);
    m_experienceWidget->setText(text.c_str());

    text = convertWithCommas(m_totalExperience);
    m_experienceTotalWidget->setText(text.c_str());
    m_sacrificeButton->enable(m_totalExperience > 0);
}

VA(0x00562660, 0x1d5)  // dc 0x1258ac
std::string convertWithCommas(long value);

void updateArtifactWidget(IconWidget* slotWidget, type_artifact artifact)
{
    if (artifact.m_artifactId == ARTIFACT_NONE) {
        slotWidget->setVisible(0);
        slotWidget->setHelpText(0, 0, 1);
    } else {
        slotWidget->setIconFrame(artifact.m_artifactId);
        slotWidget->setVisible(1);
        slotWidget->setHelpText(
            g_artifactTraits[artifact.m_artifactId].m_name, 0, 1);
    }
}

VA(0x00562840, 0x166)  // dc 0x125b3c
void SacrificeWindow::updateSlot(long slot)
{
    ArtifactSlot artifactSlot;
    memcpy(&artifactSlot, &slot, sizeof artifactSlot);
    type_artifact artifact = m_currentHero->getArtifact(artifactSlot);

    if (m_holdingArtifact.m_artifactId != ARTIFACT_NONE
        && m_currentHero->heroFn004E2840(
               m_holdingArtifact.m_artifactId, slot)) {
        updateArtifactWidget(m_slotBackWidgets[slot], artifact);

        m_slotWidgets[slot]->setVisible(1);
        m_slotWidgets[slot]->setIconFrame(SACRIFICE_ARTIFACT_SLOT_DROP_FRAME);
        m_slotWidgets[slot]->setHelpText(
            m_slotBackWidgets[slot]->getHelpText(), 0, 1);
    } else {
        m_slotBackWidgets[slot]->setVisible(0);
        updateArtifactWidget(m_slotWidgets[slot], artifact);
    }

    if (artifact.m_artifactId == ARTIFACT_NONE) {
        m_slotWidgets[slot]->setHelpText(
            g_artifactSlotTraits[slot].m_name, 0, 1);
    }
}

VA(0x005629e0, 0x33)  // dc 0x125c34
void SacrificeWindow::updateAllSlots()
{
    long slotCount = g_game->m_f1f698 >= 2 ? 19 : 18;
    for (long slot = 0; slot < slotCount; ++slot)
        updateSlot(slot);
}

VA(0x00562a20, 0x24e)  // dc 0x125c60
void SacrificeWindow::setArtifactMode()
{
    unsigned long i;
    for (i = 0; i < m_creatureWidgets.size(); ++i)
        m_creatureWidgets[i]->hide();
    for (i = 0; i < m_artifactWidgets.size(); ++i)
        m_artifactWidgets[i]->show();

    m_holdingArtifact.m_artifactId = ARTIFACT_NONE;
    updateAllSlots();

    for (i = 0; i < m_artifactOfferingWidgets.size(); ++i)
        updateArtifactOffering(i);

    for (i = 0; i < m_backpackWidgets.size(); ++i)
        updateArtifactWidget(m_backpackWidgets[i], m_currentHero->getBackpack(i));

    unsigned char scrollBackpack =
        m_currentHero->getLastBackpackIndex() + 1 > m_backpackWidgets.size();
    m_leftBackpackButton->enable(scrollBackpack);
    m_rightBackpackButton->enable(scrollBackpack);

    updateOffering(m_currentArtifactWidget, m_currentArtifactValue,
                    &m_holdingArtifact);
    m_sacrificingArtifacts = 1;
    m_emptyBackpackButton->enable(
        m_currentHero->getNumberInBackpack(1) > 0);
    m_sacrificeButton->enable(0);
    m_sacrificeButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_SACRIFICE_ARTIFACTS].m_text,
        0, 1);
    m_allArtifactsButton->enable(
        m_currentHero->getNumberInBackpack(1) > 0
        || m_currentHero->getEquippedArtifacts(1) > 0);
    m_creaturesButton->enable(m_canSacrificeCreatures);
    updateExperience();
}

VA(0x00562c70, 0x124)  // dc 0x125aac
void updateOffering(IconWidget* artifactWidget, TextWidget* valueWidget,
                     const ArtifactOffering* offering)
{
    type_artifact artifact = *offering;
    if (artifact.m_artifactId == -1) {
        artifactWidget->sendMessage(Widget::WIDGET_CLEAR_STATUS,
                                      Widget::WIDGET_DRAWN);
        artifactWidget->setHelpText(0, 0, 1);
    } else {
        artifactWidget->setIconFrame(artifact.m_artifactId);
        artifactWidget->sendMessage(Widget::WIDGET_SET_STATUS,
                                      Widget::WIDGET_DRAWN);
        artifactWidget->setHelpText(
            g_artifactTraits[artifact.m_artifactId].m_name, 0, 1);
    }

    if (offering->m_artifactId == -1) {
        artifactWidget->setHelpText(
            g_sacrificeWindowHelp[SACRIFICE_HELP_EMPTY_ARTIFACT_OFFERING].m_text,
            0, 1);
        valueWidget->setVisible(0);
        valueWidget->setHelpText(0, 0, 1);
    } else {
        valueWidget->setText(convertWithCommas(offering->m_value).c_str());
        valueWidget->setVisible(1);
        valueWidget->setHelpText(
            g_sacrificeWindowHelp[SACRIFICE_HELP_ARTIFACT_OFFERING_VALUE].m_text,
            0, 1);
    }
}

// E:\gamedcs\sacrifice_window.cpp:914
// Inlined at both retail callers. The AI-value load at +0x40 and the signed
// divide-by-forty reciprocal fix this integer value exactly.
long sacrificeValue(CreatureType creature)
{
    return g_creatureTypeTraits[creature].m_aiValue / 40 * 5;
}

// E:\gamedcs\sacrifice_window.cpp:924
// Retail proves the record layout through its six widget loads and terminal
// group/amount pair. The two general-text indices are the folded +0x1ec and
// +0x788 rows of gpGeneralText's pointer table.
// Residual (99.9744%): all 42 blocks, 20 symbolic branch targets, three
// returns and instruction counts agree. VC6
// colors four early string-return temporaries at ebp-0x30 instead of
// retail's ebp-0x40; the later help_text/result slots themselves agree.
// Dreamcast's nested line-955/958 arm scopes are restored and ratcheted even
// though their braces are byte-flat. Hoisting help_text to the procedure scope
// suggested by its raw S_REGREL32 placement is retail-refuted: it constructs
// at entry, adds cleanup paths and falls to 85.65495%. Declaration order, a
// release-VERIFY probe, and splitting total_hits declaration/assignment are
// byte-flat. A fresh why-reg pass measures 18 masked slots, finds no allocator
// model divergence, and rejects all four guided source controls (named group
// is flat; volatile available and both adjacent store swaps are worse), so
// the remaining difference is a measured C1 stack-coloring tie.
// Residual (99.9744%): with memory operands masked the two objects are
// IDENTICAL - every block, branch, call and opcode agrees and the frames are
// equal.  The only divergence is one slot: retail addresses the `result`
// string at [ebp-0x40] where this compile puts it at [ebp-0x30], sixteen
// bytes (one basic_string) apart, with `help_text` taking the other slot.
// Measured and rejected 2026-09-06: declaring `std::string result` ABOVE
// `long total_hits` in the same block, 93.8202 - it costs the `xor ebx,ebx`
// at fn+0x20 and cascades.  The two block-scoped strings do not overlay on
// either side, so the cycle is an allocation order, not a lifetime fact.
VA(0x00562da0, 0x3a2)  // dc order/name/signature + retail field graph, dc 0x125e08
void SacrificeWindow::updateCreatureOffering(
    CreatureOffering* creature)
{
    CreatureType creatureType;
    long available;
    if (creature->m_group < 0) {
        creatureType = CREATURE_NONE;
        creature->m_amount = 0;
    } else {
        creatureType = m_currentHero->m_army.m_armyTypes[creature->m_group];
        available = m_currentHero->m_army.m_numTroops[creature->m_group];
    }

    if (creatureType == CREATURE_NONE) {
        creature->m_iconWidget->setVisible(0);
        creature->m_creatureCountText->setVisible(0);
        creature->m_selectionWidget->setVisible(0);
        creature->m_experienceText->setVisible(0);
    } else {
        long totalHits = static_cast<long>(
            (sacrificeValue(creatureType) * creature->m_amount)
            * m_currentHero->getExperienceBonusFactor());
        std::string result;

        creature->m_iconWidget->setIconFrame(creatureType + 2);
        creature->m_iconWidget->setVisible(1);
        if (!creature->m_sourceSelectionFrame) {
            result = convertWithCommas(creature->m_amount);
        } else {
            result = convertWithCommas(available);
        }
        creature->m_creatureCountText->setText(result.c_str());
        creature->m_creatureCountText->setVisible(1);

        creature->m_selectionWidget->setIconFrame(creatureType + 2);
        creature->m_selectionWidget->setVisible(creature->m_amount > 0);
        result = convertWithCommas(totalHits);
        result = formatString(
            g_generalText->getText(SACRIFICE_GENERAL_TEXT_EXPERIENCE),
            result.c_str());
        creature->m_experienceText->setText(result.c_str());
        creature->m_experienceText->setVisible(creature->m_amount > 0);
    }

    if (creature->m_sourceSelectionFrame) {
        if (creatureType == CREATURE_NONE) {
            creature->m_sourceSelectionFrame->setHelpText(0, 0, 1);
            creature->m_offeringSelectionFrame->setHelpText(0, 0, 1);
        } else {
            std::string helpText;
            helpText = formatString(
                g_generalText->getText(SACRIFICE_GENERAL_TEXT_CREATURE),
                getArmyName(creatureType, 0));
            creature->m_sourceSelectionFrame->setHelpText(helpText.c_str(), 0, 1);
            creature->m_offeringSelectionFrame->setHelpText(helpText.c_str(), 0, 1);
        }
    }
}

VA(0x00563150, 0x141)  // dc 0x126064
void SacrificeWindow::setCreatureMode()
{
    unsigned long i;
    for (i = 0; i < m_artifactWidgets.size(); ++i)
        m_artifactWidgets[i]->hide();
    for (i = 0; i < m_creatureWidgets.size(); ++i)
        m_creatureWidgets[i]->show();

    for (long group = 0; group < 7; ++group) {
        m_creatureOfferings[group].m_amount = 0;
        m_creatureOfferings[group].m_group = group;
        updateCreatureOffering(&m_creatureOfferings[group]);
        m_creatureOfferings[group].m_sourceSelectionFrame->setVisible(0);
        m_creatureOfferings[group].m_offeringSelectionFrame->setVisible(0);
    }

    m_currentCreature.m_group = -1;
    updateCreatureOffering(&m_currentCreature);
    m_sacrificingArtifacts = 0;
    m_maxCreaturesButton->enable(0);
    m_creatureSlider->enable(0);
    m_sacrificeButton->enable(0);
    m_sacrificeButton->setHelpText(
        g_sacrificeWindowHelp[SACRIFICE_HELP_SACRIFICE_CREATURES].m_text,
        0, 1);
    m_creatureNameWidget->setVisible(0);
    m_allCreaturesButton->enable(
        m_currentHero->m_army.getCreatureTotal() > 1);
    m_artifactsButton->enable(m_canSacrificeArtifacts);
    updateExperience();
}

// E:\gamedcs\sacrifice_window.cpp:1036
// Retail expands this helper at the click sites. The DC line map fixes the
// source order and argument ABI; the Complete mouse-pointer and refresh call
// graph independently proves the body.
void SacrificeWindow::pickUpArtifact(
    type_artifact artifact, long slot, unsigned char newArtifact)
{
    m_holdingArtifact.set(&artifact, slot, m_currentHero);
    if (newArtifact) {
        m_totalExperience += m_holdingArtifact.m_value;
        updateExperience();
    }
    updateOffering(m_currentArtifactWidget, m_currentArtifactValue,
                    &m_holdingArtifact);
    g_mouseManager->setPointer(m_holdingArtifact.m_artifactId,
                               MouseManager::ARTIFACT_SET);
    updateAllSlots();
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

// E:\gamedcs\sacrifice_window.cpp:1053
// The Dreamcast line table and xref graph prove this helper boundary at each
// artifact-drop site. Complete folds the false change-experience arm into
// sacrifice, but retains the helper's redraw as a distinct inline tail.
void SacrificeWindow::putDownArtifact(
    unsigned char changeExperience)
{
    if (changeExperience) {
        m_totalExperience -= m_holdingArtifact.m_value;
        updateExperience();
    }
    m_holdingArtifact.m_artifactId = ARTIFACT_NONE;
    updateOffering(m_currentArtifactWidget, m_currentArtifactValue,
                    &m_holdingArtifact);
    g_mouseManager->setPointer(0, MouseManager::DEFAULT_SET);
    updateAllSlots();
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

VA(0x005632a0, 0x417)  // dc 0x1262e4
void SacrificeWindow::artifactClick(
    long slot, unsigned char rightClick)
{
    type_artifact oldArtifact = m_currentHero->getArtifact(ArtifactSlot(slot));

    if (m_holdingArtifact.m_artifactId == ARTIFACT_NONE) {
        if (oldArtifact.m_artifactId == ARTIFACT_NONE)
            return;

        if (rightClick) {
            m_currentHero->viewArtifact(&oldArtifact, rightClick);
            return;
        }

        if (oldArtifact.m_artifactId == ARTIFACT_SPELLBOOK) {
            SpellbookWindow spellbook(
                *m_currentHero, 0, SpellbookWindow::eContextNeither,
                m_currentHero->getSpecialTerrain());
            spellbook.doModal(0);
            return;
        }

        if (oldArtifact.m_artifactId == ARTIFACT_CATAPULT) {
            normalDialog(
                g_generalText->getText(
                    SACRIFICE_GENERAL_TEXT_CANNOT_SACRIFICE_ARTIFACT),
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            return;
        }

        m_currentHero->removeArtifact(slot);
        updateSlot(slot);
        pickUpArtifact(oldArtifact, slot, 1);
        return;
    }

    if (rightClick)
        return;
    if (!m_currentHero->heroFn004E2840(
            m_holdingArtifact.m_artifactId, slot))
        return;

    if (oldArtifact.m_artifactId != ARTIFACT_NONE)
        m_currentHero->removeArtifact(slot);
    m_currentHero->equipArtifact(&m_holdingArtifact, slot);
    updateSlot(slot);
    putDownArtifact(1);
    if (oldArtifact.m_artifactId != ARTIFACT_NONE)
        pickUpArtifact(oldArtifact, slot, 1);
}

// E:\gamedcs\sacrifice_window.cpp:1127
// Complete expands this helper into both halves of backpack_click. Its widget
// vector walk and shared scrolling-state byte are directly visible there.
void SacrificeWindow::updateBackpack()
{
    for (unsigned long i = 0; i < m_backpackWidgets.size(); ++i)
        updateArtifactWidget(m_backpackWidgets[i],
                               m_currentHero->getBackpack(i));

    unsigned char scrollBackpack =
        m_currentHero->getLastBackpackIndex() + 1
        > m_backpackWidgets.size();
    m_leftBackpackButton->enable(scrollBackpack);
    m_rightBackpackButton->enable(scrollBackpack);
}

// Picking up an artifact removes its backpack record. Putting one down inserts
// it or displays the backpack error.
// E:\gamedcs\sacrifice_window.cpp:1150
VA(0x005636c0, 0x31a)  // widget call edge + dc name/order, dc 0x1264dc
void SacrificeWindow::backpackClick(
    long slot, unsigned char rightClick)
{
    type_artifact oldArtifact = m_currentHero->getBackpack(slot);

    if (m_holdingArtifact.m_artifactId == ARTIFACT_NONE) {
        if (oldArtifact.m_artifactId != ARTIFACT_NONE) {
            if (rightClick) {
                m_currentHero->viewArtifact(
                    &oldArtifact, rightClick);
            } else {
                m_currentHero->removeBackpackArtifact(slot);
                updateBackpack();
                pickUpArtifact(oldArtifact, 19, 1);
            }
        }
    } else if (!rightClick) {
        if (!m_currentHero->addToBackpack(&m_holdingArtifact, slot)) {
            normalDialog(
                m_currentHero
                    ->getBackpackError(
                        m_holdingArtifact.m_artifactId)
                    .c_str(),
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        } else {
            updateBackpack();
            putDownArtifact(1);
        }
    }
}

VA(0x005639e0, 0x5b)  // dc 0x125a4c
void updateArtifactWidget(IconWidget* slotWidget, type_artifact artifact);

VA(0x00563a40, 0x31)  // dc 0x1265b8
void SacrificeWindow::updateArtifactOffering(long slot)
{
    updateOffering(m_artifactOfferingWidgets[slot],
                    m_artifactValueWidgets[slot],
                    &m_artifactOfferings[slot]);
}

VA(0x00563a80, 0x31b)  // dc 0x126640
void SacrificeWindow::offeringClick(
    long slot, unsigned char rightClick)
{
    ArtifactOffering oldArtifact = m_artifactOfferings[slot];

    if (m_holdingArtifact.m_artifactId == ARTIFACT_NONE) {
        if (oldArtifact.m_artifactId != ARTIFACT_NONE) {
            if (rightClick) {
                m_currentHero->viewArtifact(
                    &oldArtifact, rightClick);
            } else {
                m_artifactOfferings[slot].m_artifactId = ARTIFACT_NONE;
                updateArtifactOffering(slot);
                pickUpArtifact(
                    oldArtifact, oldArtifact.m_source, 0);
            }
        }
    } else if (!rightClick) {
        m_artifactOfferings[slot] = m_holdingArtifact;
        updateArtifactOffering(slot);
        putDownArtifact(0);
        if (oldArtifact.m_artifactId != ARTIFACT_NONE)
            pickUpArtifact(oldArtifact, oldArtifact.m_source, 0);
    }
}

VA(0x00563da0, 0x152)
int SacrificeWindow::scrollBackpackLeft(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[
                SACRIFICE_HELP_SCROLL_BACKPACK_LEFT].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SacrificeWindow* window =
            static_cast<SacrificeWindow*>(msg.m_window);
        window->m_currentHero->rotateBackpackLeft();
        window->updateBackpack();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00563f00, 0x152)
int SacrificeWindow::scrollBackpackRight(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[
                SACRIFICE_HELP_SCROLL_BACKPACK_RIGHT].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SacrificeWindow* window =
            static_cast<SacrificeWindow*>(msg.m_window);
        window->m_currentHero->rotateBackpackRight();
        window->updateBackpack();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

// E:\gamedcs\sacrifice_window.cpp:1289
// Complete expands this helper into both artifact-batch callbacks. It fills
// the first empty offering, adds that record's scaled value, and refreshes
// the corresponding pair of offering widgets.
unsigned char SacrificeWindow::addArtifact(
    type_artifact artifact, long source)
{
    unsigned long i;
    for (i = 0; i < m_artifactOfferings.size(); ++i) {
        if (m_artifactOfferings[i].m_artifactId == ARTIFACT_NONE)
            break;
    }
    if (i == m_artifactOfferings.size())
        return 0;

    m_artifactOfferings[i].set(&artifact, source, m_currentHero);
    m_totalExperience += m_artifactOfferings[i].m_value;
    updateArtifactOffering(i);
    return 1;
}

// E:\gamedcs\sacrifice_window.cpp:1310
// The helper is expanded at each callback. Retail scans the fixed 64-record
// backpack for its next occupied slot and stops if the offering pane fills.
void SacrificeWindow::emptyBackpack()
{
    type_artifact artifact;
    while (m_currentHero->getNumberInBackpack(1) > 0) {
        long i;
        for (i = 0; i < SACRIFICE_BACKPACK_ARTIFACT_COUNT; ++i) {
            artifact = m_currentHero->getBackpack(i);
            if (artifact.m_artifactId != ARTIFACT_NONE)
                break;
        }
        if (!addArtifact(artifact, SACRIFICE_BACKPACK_SOURCE_SLOT))
            break;
        m_currentHero->removeBackpackArtifact(i);
    }
    updateBackpack();
}

VA(0x00564060, 0x2d3)
int SacrificeWindow::emptyBackpack(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[SACRIFICE_HELP_EMPTY_BACKPACK].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SacrificeWindow* window =
            static_cast<SacrificeWindow*>(msg.m_window);
        window->emptyBackpack();
        window->updateExperience();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

// E:\gamedcs\sacrifice_window.cpp:1365
// The all-artifacts callback first offers each of the sixteen admissible doll
// slots, then expands empty_backpack to fill whatever offering slots remain.
// NOT A MEMBER-OFFSET BUG (checked 2026-09-06): the `[eax+0x2e4]` against
// retail's `[eax+0x350]` that a census flagged here is the SWITCH INDEX
// TABLE - `mov dl, byte ptr [eax + <fn>+0x350]` / `jmp [4*edx + <fn>+0x33c]`
// - whose base is the function's own end.  Retail's body is 0x6c bytes
// longer than ours by exactly the update_backpack expansion below, so the
// two tables sit 0x6c apart.  No hero/type_sacrifice_window member is
// involved.
// Residual (86.78%): the first 37 semantic blocks agree. This compile expands
// empty_backpack but keeps its nested update_backpack call, whereas retail
// expands both. An ordinary inline hint is byte-flat; force-inlining either
// helper improves this site to about 91.4% but regresses the exact standalone
// empty-backpack callback (and force-inlining update_backpack also regresses
// backpack_click), so the source-authentic call graph is retained.
VA(0x00564340, 0x35f)  // callback address-take + dc name/signature/order
int SacrificeWindow::allArtifacts(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[SACRIFICE_HELP_ALL_ARTIFACTS].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SacrificeWindow* window =
            static_cast<SacrificeWindow*>(msg.m_window);
        type_artifact artifact;
        for (long slot = 0; slot < SACRIFICE_EQUIPPED_SLOT_COUNT; ++slot) {
            artifact = window->m_currentHero->getArtifact(ArtifactSlot(slot));
            if (artifact.m_artifactId != ARTIFACT_NONE) {
                if (!window->addArtifact(artifact, slot))
                    break;
                window->m_currentHero->removeArtifact(slot);
                window->updateSlot(slot);
            }
        }
        window->emptyBackpack();
        window->updateExperience();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x005646a0, 0x269)
int SacrificeWindow::sacrifice(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[
                SACRIFICE_HELP_SACRIFICE_ARTIFACTS].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SacrificeWindow* window =
            static_cast<SacrificeWindow*>(msg.m_window);
        if (window->m_sacrificingArtifacts) {
            for (unsigned long i = 0;
                 i < window->m_artifactOfferings.size(); ++i) {
                window->m_artifactOfferings[i].m_artifactId = ARTIFACT_NONE;
                window->updateArtifactOffering(i);
            }
            if (window->m_holdingArtifact.m_artifactId != ARTIFACT_NONE)
                window->putDownArtifact(0);
        } else {
            ArmyGroup* army = &window->m_currentHero->m_army;
            long group;
            for (group = 0;
                 group < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++group) {
                army->m_numTroops[group] -=
                    window->m_creatureOfferings[group].m_amount;
                if (army->m_numTroops[group] <= 0)
                    army->dismiss(group);
                window->m_creatureOfferings[group].m_amount = 0;
                window->updateCreatureOffering(
                    &window->m_creatureOfferings[group]);
                window->m_creatureOfferings[group].m_sourceSelectionFrame->setVisible(0);
                window->m_creatureOfferings[group].m_offeringSelectionFrame->setVisible(0);
            }
            window->m_currentCreature.m_group = -1;
            window->updateCreatureOffering(&window->m_currentCreature);
            window->m_creatureNameWidget->setVisible(0);
            window->m_allCreaturesButton->enable(
                army->getCreatureTotal() > 1);
            window->m_creatureSlider->setResolution(1);
            window->m_creatureSlider->setState(0);
            window->m_creatureSlider->enable(0);
            window->m_maxCreaturesButton->enable(0);
        }

        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        window->m_currentHero->giveExperience(
            window->m_totalExperience, 1, 1);
        window->m_totalExperience = 0;
        window->updateExperience();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00564910, 0x164)
int SacrificeWindow::sacrificeCreatures(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[
                SACRIFICE_HELP_SACRIFICE_CREATURES_BUTTON].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SacrificeWindow* window =
            static_cast<SacrificeWindow*>(msg.m_window);
        window->clear();
        window->setCreatureMode();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

// The Dreamcast line map places this one-purpose helper at source line 1499.
// Complete expands it into clear. The same order is independently repeated
// by ExitDialog: original equipped slot, any legal equipped slot, backpack,
// then a final arbitrary equipped-slot fallback.
void SacrificeWindow::returnArtifact(
    const ArtifactOffering& artifact)
{
    if (artifact.m_source < 19) {
        if (m_currentHero->equipArtifact(&artifact, artifact.m_source))
            return;
        if (m_currentHero->equipArtifact(&artifact, -1))
            return;
    }
    if (m_currentHero->addToBackpack(&artifact, -1))
        return;
    m_currentHero->equipArtifact(&artifact, -1);
}

// The Dreamcast line map places clear at source line 1521. Complete expands
// it into sacrifice_artifacts; retail's vector walk, held-record cleanup and
// terminal total reset expose the entire body.
void SacrificeWindow::clear()
{
    for (unsigned long i = 0; i < m_artifactOfferings.size(); ++i) {
        if (m_artifactOfferings[i].m_artifactId != ARTIFACT_NONE) {
            returnArtifact(m_artifactOfferings[i]);
            m_artifactOfferings[i].m_artifactId = ARTIFACT_NONE;
        }
    }

    if (m_holdingArtifact.m_artifactId != ARTIFACT_NONE) {
        returnArtifact(m_holdingArtifact);
        m_holdingArtifact.m_artifactId = ARTIFACT_NONE;
    }
    m_totalExperience = 0;
}

VA(0x00564a80, 0x171)
int SacrificeWindow::exitClick(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[SACRIFICE_HELP_EXIT_BUTTON].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SacrificeWindow* window =
            static_cast<SacrificeWindow*>(msg.m_window);
        window->clear();
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = 0;
        msg.m_codeY = Widget::WIDGET_END_DIALOG;
        msg.m_codeX = Widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x00564c00, 0xe6)
void SacrificeWindow::setCreatureSacrifice(long slot, long newAmount)
{
    if (m_creatureOfferings[slot].m_amount == newAmount)
        return;

    CreatureType creatureType = m_currentHero->m_army.m_armyTypes[slot];
    long value = sacrificeValue(creatureType);
    long oldExperience = static_cast<long>(
        (value * m_creatureOfferings[slot].m_amount)
        * m_currentHero->getExperienceBonusFactor());
    m_totalExperience += static_cast<long>(
        (value * newAmount) * m_currentHero->getExperienceBonusFactor())
        - oldExperience;

    m_creatureOfferings[slot].m_amount = newAmount;
    updateCreatureOffering(&m_creatureOfferings[slot]);
    if (m_currentCreature.m_group == slot) {
        m_currentCreature.m_amount = newAmount;
        updateCreatureOffering(&m_currentCreature);
    }
}

// E:\gamedcs\sacrifice_window.cpp:1599
// Retail expands this helper at both callers. The scan preserves one troop
// only when every other army slot has already been offered to its limit.
long SacrificeWindow::getMaxAmount(long slot) const
{
    long amount = m_currentHero->m_army.m_numTroops[slot];
    if (amount <= 0)
        return 0;

    long other;
    for (other = 0; other < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++other) {
        if (other != slot
            && m_creatureOfferings[other].m_amount
                < m_currentHero->m_army.m_numTroops[other])
            break;
    }
    if (other == ArmyGroup::ARMY_GROUP_SLOT_COUNT)
        --amount;
    return amount;
}

VA(0x00564cf0, 0xe9)
int SacrificeWindow::allCreatures(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[SACRIFICE_HELP_ALL_CREATURES].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SacrificeWindow* window =
            static_cast<SacrificeWindow*>(msg.m_window);
        long slot = ArmyGroup::ARMY_GROUP_SLOT_COUNT - 1;
        do {
            window->setCreatureSacrifice(
                slot, window->getMaxAmount(slot));
        } while (slot--);
        window->m_creatureSlider->setState(
            window->m_creatureSlider->getMaximum() - 1);
        window->updateExperience();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00564de0, 0x88)
int SacrificeWindow::maxCreatures(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[SACRIFICE_HELP_MAX_CREATURES].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SacrificeWindow* window =
            static_cast<SacrificeWindow*>(msg.m_window);
        int maximum = window->m_creatureSlider->getMaximum() - 1;
        window->m_creatureSlider->setState(maximum);
        creatureSliderChange(maximum, window);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00564e70, 0x164)
int SacrificeWindow::sacrificeArtifacts(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT) {
        normalDialog(
            g_sacrificeWindowHelp[
                SACRIFICE_HELP_SACRIFICE_ARTIFACTS_BUTTON].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SacrificeWindow* window =
            static_cast<SacrificeWindow*>(msg.m_window);
        window->clear();
        window->setArtifactMode();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00564fe0, 0x394)  // dc 0x1270f0
void SacrificeWindow::creatureClick(
    long slot, unsigned char rightClick, unsigned char leftPane)
{
    if (rightClick || slot == m_currentCreature.m_group || slot < 0) {
        if (slot < 0)
            slot = m_currentCreature.m_group;
        if (slot < 0) {
            if (rightClick) {
                normalDialog(
                    g_generalText->getText(
                        SACRIFICE_GENERAL_TEXT_EMPTY_CREATURE),
                    4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            }
            return;
        }

        CreatureType creatureType = m_currentHero->m_army.m_armyTypes[slot];
        long amount = m_creatureOfferings[slot].m_amount;
        if (leftPane)
            amount = m_currentHero->m_army.m_numTroops[slot] - amount;
        if (creatureType != CREATURE_NONE && amount > 0) {
            ViewArmyWindow viewArmyWindow(
                creatureType, 0x77, 0x20,
                static_cast<unsigned char>(!rightClick));
            viewArmyWindow.centerWindow(-1, -1);
            if (rightClick)
                viewArmyWindow.quickView();
            else
                viewArmyWindow.doModal();
        }
    } else {
        if (m_currentCreature.m_group >= 0) {
            m_creatureOfferings[m_currentCreature.m_group].m_offeringSelectionFrame->setVisible(0);
            m_creatureOfferings[m_currentCreature.m_group].m_sourceSelectionFrame->setVisible(0);
        }

        m_currentCreature.m_group = slot;
        m_currentCreature.m_amount = m_creatureOfferings[slot].m_amount;
        updateCreatureOffering(&m_currentCreature);
        updateCreatureOffering(&m_creatureOfferings[slot]);

        if (m_currentHero->m_army.m_armyTypes[slot] == CREATURE_NONE) {
            m_creatureNameWidget->setVisible(0);
        } else {
            std::string buffer;
            buffer = formatString(
                g_generalText->getText(
                    SACRIFICE_GENERAL_TEXT_CREATURE_NAME),
                getArmyName(m_currentHero->m_army.m_armyTypes[slot], 0));
            m_creatureNameWidget->setText(buffer.c_str());
            m_creatureNameWidget->setVisible(1);
            m_creatureOfferings[slot].m_offeringSelectionFrame->setVisible(1);
            m_creatureOfferings[slot].m_sourceSelectionFrame->setVisible(1);
        }

        long maximum = getMaxAmount(slot);
        m_creatureSlider->setResolution(maximum + 1);
        m_creatureSlider->setState(m_currentCreature.m_amount);
        m_creatureSlider->enable(maximum > 0);
        m_maxCreaturesButton->enable(maximum > 0);
        drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    }
}

VA(0x00565380, 0x2e)
void SacrificeWindow::creatureSliderChange(
    int state, HeroWindow* parentWindow)
{
    SacrificeWindow* window =
        static_cast<SacrificeWindow*>(parentWindow);
    window->setCreatureSacrifice(window->m_currentCreature.m_group, state);
    window->updateExperience();
    window->drawWindow(
        1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

VA(0x005653b0, 0x37)  // dc 0x127404
int SacrificeWindow::doModal(unsigned char fadeIn)
{
    if (m_canSacrificeArtifacts)
        setArtifactMode();
    else
        setCreatureMode();
    return HeroWindow::doModal(fadeIn);
}

VA(0x005653f0, 0x3b)  // dc 0x12743c
void SacrificeWindow::handleWidgetHover(Widget* currentWidget)
{
    if (!currentWidget->getHelpText())
        m_rolloverText->setText(g_emptyRolloverText);
    else
        m_rolloverText->setText(currentWidget->getHelpText());
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

// E:\gamedcs\sacrifice_window.cpp:1859
// Vtable 0x641620 slot 14 fixes the identity. Complete returns any held
// artifact to its original equipped slot where possible, then tries an
// arbitrary equipped slot, the backpack, and finally the arbitrary equipped
// path once more before closing the modal dialog.
// DC line 1866 calls return_artifact. The existing ordinary returnArtifact
// expands naturally here and preserves 100% while removing three failure joins.
VA(0x00565430, 0x80)  // anchor-vtable slot 14, dc 0x1274bc
int SacrificeWindow::exitDialog(Message& msg)
{
    ArtifactOffering* artifact = &m_holdingArtifact;
    msg.m_id = MESSAGE_WIDGET;
    g_windowManager->m_dialogReturn = 0;
    msg.m_codeY = Widget::WIDGET_END_DIALOG;
    msg.m_codeX = Widget::WIDGET_END_DIALOG;

    if (artifact->m_artifactId != -1) {
        returnArtifact(*artifact);
        artifact->m_artifactId = ARTIFACT_NONE;
    }
    return MESSAGE_DISPATCH_FORWARD;
}

VA_COMPGEN(0x005654b0, 0x5, IMPLICIT_DTOR, TransformerSlot)  // dc 0x128764

// The public UAA_N_N0 signature preserves native Boolean click values.
VA(0x005654c0, 0x2a)  // linkorder + the +0x48/+0x4c pair, dc 0x127598
bool TransformerSlot::handleClick(
    bool downClick, bool rightClick)
{
    if (downClick) {
        static_cast<SkeletonWindow*>(m_parentWindow)->creatureClick(
            m_group, m_slot, rightClick);
        return 1;
    }
    return 0;
}

// E:\gamedcs\sacrifice_window.cpp:1891
// Complete expands both calls in the transformer icon grid and retains no
// separately claimable constructor body.
TransformerSlot::TransformerSlot(
    long newX, long newY, long newW, long newH, long newGroup,
    long newSlot, long newId, const char* image)
    : IconWidget(newX, newY, newW, newH, newId, image,
                 0, 0, 0, 0, 16)
{
    m_slot = newSlot;
    m_group = newGroup;
}

// The transformer dialog's own pair, found by the same vtable-uniqueness
// scan that carried the tradpost family: 0x565f30 is a 33-byte scalar
// deleting destructor whose only image-wide reference is slot 0 of vtable
// 0x641694, and its callee 0x565f60 stores that same vtable. The vtable
// itself is referenced exactly twice - here and in the 0xa3c constructor at
// 0x5654f0 - so neither row is an /OPT:ICF fold.
VA_COMPGEN(0x00565f30, 0x21, SCALAR_DELETING_DTOR, SkeletonWindow)

VA(0x005654f0, 0xA3C)  // dc 0x1275c0
SkeletonWindow::SkeletonWindow(ArmyGroup* newArmy)
    : CAdvPopup(100, 67, 600, 485, 18)
{
    long widgetId = 100;
    m_selectedCreatures.initialize();
    m_armies[0] = newArmy;
    m_armies[1] = &m_selectedCreatures;
    m_selectedGroup = -1;
    m_selectedIndex = -1;

    BitmapBorder* background = new BitmapBorder(
        0, 0, 600, 485, widgetId++, "SkTrnBk.pcx", 0x800);
    background->setPlayerPaletteColors(
        g_game->getLocalPlayerGamePos());
    m_widgets.push_back(background);

    std::vector<Widget*>& widgets = m_widgets;
    widgets.insert(widgets.end(), new TextWidget(
        25, 21, 257, 18,
        g_generalText->getText(
            SACRIFICE_GENERAL_TEXT_TRANSFORMER_SOURCE_TITLE),
        "smalfont.fnt", Font::HEADING, -1, 1, 0, 8));
    m_widgets.push_back(new TextWidget(
        320, 21, 257, 18,
        g_generalText->getText(
            SACRIFICE_GENERAL_TEXT_TRANSFORMER_DESTINATION_TITLE),
        "smalfont.fnt", Font::HEADING, -1, 1, 0, 8));
    m_widgets.push_back(new TextWidget(
        25, 55, 257, 42,
        g_generalText->getText(
            SACRIFICE_GENERAL_TEXT_TRANSFORMER_SOURCE_DESCRIPTION),
        "medfont.fnt", Font::HEADING, -1, 1, 0, 8));
    m_widgets.push_back(new TextWidget(
        320, 55, 257, 42,
        g_generalText->getText(
            SACRIFICE_GENERAL_TEXT_TRANSFORMER_DESTINATION_DESCRIPTION),
        "medfont.fnt", Font::HEADING, -1, 1, 0, 8));

    createCreatureIcons(
        45, 109, 3, 2, 0, 0, widgetId,
        &m_armyWidget[0][0], &m_selectBorder[0][0], &m_armyLabel[0][0]);
    createCreatureIcons(
        128, 305, 1, 1, 0, 6, widgetId,
        &m_armyWidget[0][6], &m_selectBorder[0][6], &m_armyLabel[0][6]);
    createCreatureIcons(
        334, 109, 3, 2, 1, 0, widgetId,
        &m_armyWidget[1][0], &m_selectBorder[1][0], &m_armyLabel[1][0]);
    createCreatureIcons(
        417, 305, 1, 1, 1, 6, widgetId,
        &m_armyWidget[1][6], &m_selectBorder[1][6], &m_armyLabel[1][6]);

    m_allCreaturesButton = new FuncButton(
        146, 416, 64, 32, widgetId++, "AltArmy.def",
        allCreatures, 0, 1);
    m_allCreaturesButton->setHelpText(
        g_transformerWindowHelp[TRANSFORMER_HELP_ALL_CREATURES].m_text,
        0, 1);
    m_widgets.push_back(m_allCreaturesButton);

    m_sacrificeButton = new FuncButton(
        269, 416, 64, 32, widgetId++, "AltSacr.def",
        sacrifice, 0, 1);
    m_sacrificeButton->setHelpText(
        g_transformerWindowHelp[TRANSFORMER_HELP_SACRIFICE].m_text,
        0, 1);
    m_sacrificeButton->enable(0);
    m_widgets.push_back(m_sacrificeButton);

    FuncButton* exitButton = new FuncButton(
        392, 416, 64, 32, widgetId++, "iCancel.def",
        exitClick, 0, 1);
    exitButton->setHelpText(
        g_transformerWindowHelp[TRANSFORMER_HELP_EXIT].m_text, 0, 1);
    m_widgets.push_back(exitButton);

    m_rolloverText = new TextWidget(
        8, 459, 585, 19, g_emptyRolloverText, "smalfont.fnt",
        Font::PRIMARY, widgetId++, 1, 0, 8);
    m_widgets.push_back(m_rolloverText);

    for (long i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++i)
        update(0, i);
    addWidgetsToMessageStream();
}

VA(0x00565f60, 0xC2)  // dc 0x127a08
SkeletonWindow::~SkeletonWindow()
{
    for (unsigned int i = 0; i < m_deathSamples.size(); i++) {
        g_soundManager->stopSample(m_deathSamples[i]->m_memSample.m_memSampleHandle);
        m_deathSamples[i]->dispose();
    }
    deleteWidgets();
}

// E:\gamedcs\sacrifice_window.cpp:2157
// All Complete callers inline this source helper. The DC call edges and the
// repeated retail expansion prove the transformed-army scan and the two
// terminal button states.
inline void SkeletonWindow::updateButtons()
{
    long i;
    for (i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
        long type = m_armies[1]->m_armyTypes[i];
        if (type == CREATURE_NONE)
            continue;
        if (type != g_deathCreature[type])
            break;
    }
    m_sacrificeButton->enable(i < ArmyGroup::ARMY_GROUP_SLOT_COUNT);
    m_allCreaturesButton->enable(m_armies[0]->hasCreatures());
}

VA(0x00566030, 0x45D)  // dc 0x127b68
void SkeletonWindow::update(long group, long index)
{
    CreatureType type = m_armies[group]->m_armyTypes[index];
    if (type == CREATURE_NONE) {
        m_armyWidget[group][index]->setVisible(0);
        m_armyLabel[group][index]->setVisible(0);
        m_selectBorder[group][index]->setHelpText(0, 0, 1);
        m_armyWidget[group][index]->setHelpText(0, 0, 1);
        m_armyLabel[group][index]->setHelpText(0, 0, 1);
        return;
    }

    std::string result;
    const char* name = getArmyName(
        type, m_armies[group]->m_numTroops[index]);

    m_armyWidget[group][index]->setIconFrame(type + 2);
    result = formatString(
        DATA_COMPGEN(0x00660a1c, decimalFormat, "%d"),
        m_armies[group]->m_numTroops[index]);
    m_armyLabel[group][index]->setText(result.c_str());
    m_armyWidget[group][index]->setVisible(1);
    m_armyLabel[group][index]->setVisible(1);

    if (group == 0) {
        result = formatString(
            g_generalText->getText(SACRIFICE_GENERAL_TEXT_CREATURE), name);
    } else {
        int transformed = g_deathCreature[type];
        if (transformed != type) {
            result = formatString(
                g_generalText->getText(
                    SACRIFICE_GENERAL_TEXT_TRANSFORM_CREATURE),
                name, getArmyName(
                    transformed, m_armies[group]->m_numTroops[index]));
        } else if (m_armies[group]->m_numTroops[index] == 1) {
            result = formatString(
                g_generalText->getText(
                    SACRIFICE_GENERAL_TEXT_ALREADY_TRANSFORMED_ONE),
                name);
        } else {
            result = formatString(
                g_generalText->getText(
                    SACRIFICE_GENERAL_TEXT_ALREADY_TRANSFORMED_MANY),
                name);
        }
    }

    m_armyWidget[group][index]->setHelpText(result.c_str(), 0, 1);
    m_selectBorder[group][index]->setHelpText(result.c_str(), 0, 1);
    result = formatString(
        DATA_COMPGEN(0x006778a4, resourceQuantityFormat, "%d %s"),
        m_armies[group]->m_numTroops[index], name);
    m_armyLabel[group][index]->setHelpText(result.c_str(), 0, 1);
}

VA(0x00566490, 0x258)
void SkeletonWindow::creatureClick(
    long side, long slot, unsigned char rightClick)
{
    CreatureType creatureType = m_armies[side]->m_armyTypes[slot];

    if (rightClick
        || (slot == m_selectedIndex && side == m_selectedGroup)) {
        if (creatureType != CREATURE_NONE) {
            ViewArmyWindow viewArmyWindow(
                creatureType, 0x77, 0x20,
                static_cast<unsigned char>(!rightClick));
            viewArmyWindow.centerWindow(-1, -1);
            if (rightClick)
                viewArmyWindow.quickView();
            else
                viewArmyWindow.doModal();
        }
    } else if (m_selectedGroup < 0) {
        m_selectedIndex = slot;
        m_selectedGroup = side;
        m_selectBorder[side][slot]->sendMessage(
            Widget::WIDGET_SET_STATUS, Widget::WIDGET_DRAWN);
        m_selectBorder[side][slot]->draw();
        drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    } else {
        if (creatureType
            == m_armies[m_selectedGroup]->m_armyTypes[m_selectedIndex]) {
            m_armies[side]->add(
                creatureType,
                m_armies[m_selectedGroup]->m_numTroops[m_selectedIndex], slot);
            m_armies[m_selectedGroup]->dismiss(m_selectedIndex);
        } else {
            long troops = m_armies[side]->m_numTroops[slot];
            m_armies[side]->m_armyTypes[slot] =
                m_armies[m_selectedGroup]->m_armyTypes[m_selectedIndex];
            m_armies[side]->m_numTroops[slot] =
                m_armies[m_selectedGroup]->m_numTroops[m_selectedIndex];
            m_armies[m_selectedGroup]->m_armyTypes[m_selectedIndex] = creatureType;
            m_armies[m_selectedGroup]->m_numTroops[m_selectedIndex] = troops;
        }
        m_selectBorder[m_selectedGroup][m_selectedIndex]->sendMessage(
            Widget::WIDGET_CLEAR_STATUS, Widget::WIDGET_DRAWN);
        update(side, slot);
        update(m_selectedGroup, m_selectedIndex);

        Widget::clearHoverWidget();
        m_selectedGroup = -1;
        m_selectedIndex = -1;

        updateButtons();
        drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    }
}

// E:\gamedcs\sacrifice_window.cpp:2281
// Slot 9, sitting two rows past creature_click 0x566490 - the call target the
// transformer slot above pins - in the Dreamcast roster's order.
VA(0x005666f0, 0x2e)  // anchor-callee (CAdvPopup slot 9) + linkorder, dc 0x128048
int SkeletonWindow::windowHandler(Message& msg)
{
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;
    if (msg.m_id == MESSAGE_MOUSE_MOVE)
        return g_windowManager->convertToHover(msg);
    return 0;
}

VA(0x00566720, 0x38)  // dc 0x128098
void SkeletonWindow::handleWidgetHover(Widget* currentWidget)
{
    if (!currentWidget->getHelpText())
        m_rolloverText->setText(g_emptyRolloverText);
    else
        m_rolloverText->setText(currentWidget->getHelpText());
    drawWindow(1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
}

VA(0x00566760, 0x28f)  // dc 0x1280e0
void SkeletonWindow::createCreatureIcons(
    long iconX, long iconY, long columns, long rows,
    long groupNumber, long itemNumber, long& widgetId,
    IconWidget** iconWidgets, IconWidget** selectionWidgets,
    TextWidget** textWidgets)
{
    long row;
    long count = 0;
    long column;
    long textX = iconX - 5;
    long textY = iconY + 68;

    for (row = 0; row < rows; ++row) {
        for (column = 0; column < columns; ++column) {
            textWidgets[count] = new TextWidget(
                textX, textY, 66, 16, g_emptyRolloverText,
                "smalfont.fnt", Font::PRIMARY, widgetId++, 1, 0, 8);
            m_widgets.push_back(textWidgets[count]);

            iconWidgets[count] = new TransformerSlot(
                iconX, iconY, 58, 64, groupNumber, itemNumber + count,
                widgetId++, "twcrport.def");
            m_widgets.push_back(iconWidgets[count]);

            selectionWidgets[count] = new TransformerSlot(
                iconX, iconY, 58, 64, groupNumber, itemNumber + count,
                widgetId++, "TwCrPort.def");
            m_widgets.push_back(selectionWidgets[count]);
            selectionWidgets[count]->setIconFrame(1);
            selectionWidgets[count]->sendMessage(
                Widget::WIDGET_CLEAR_STATUS, Widget::WIDGET_DRAWN);

            ++count;
            textX += 83;
            iconX += 83;
        }
        textX -= columns * 83;
        iconX -= columns * 83;
        textY += 98;
        iconY += 98;
    }
}

// E:\gamedcs\sacrifice_window.cpp:2385
// DC names this helper and records both transformer callbacks as callers.
// Complete expands both calls: occupied source slots move into the same
// destination slot when free, otherwise armyGroup::Add chooses a slot.
inline void moveAllArmies(ArmyGroup* source, ArmyGroup* dest)
{
    for (long i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
        if (source->m_armyTypes[i] == CREATURE_NONE)
            continue;
        long destIndex = i;
        if (dest->m_armyTypes[i] != CREATURE_NONE)
            destIndex = -1;
        dest->add(source->m_armyTypes[i], source->m_numTroops[i], destIndex);
        source->dismiss(i);
    }
}

VA(0x005669f0, 0x137)  // dc 0x128310
int SkeletonWindow::allCreatures(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT
        && (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        normalDialog(
            g_transformerWindowHelp[TRANSFORMER_HELP_ALL_CREATURES].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SkeletonWindow* window =
            static_cast<SkeletonWindow*>(msg.m_window);
        moveAllArmies(window->m_armies[0], window->m_armies[1]);
        for (long i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
            window->update(0, i);
            window->update(1, i);
        }
        window->updateButtons();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

VA(0x00566b30, 0xE4)  // dc 0x1283ac
int SkeletonWindow::exitClick(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT
        && (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        normalDialog(
            g_transformerWindowHelp[TRANSFORMER_HELP_EXIT].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SkeletonWindow* window =
            static_cast<SkeletonWindow*>(msg.m_window);
        moveAllArmies(window->m_armies[1], window->m_armies[0]);
        msg.m_id = MESSAGE_WIDGET;
        g_windowManager->m_dialogReturn = 0;
        msg.m_codeY = Widget::WIDGET_END_DIALOG;
        msg.m_codeX = Widget::WIDGET_END_DIALOG;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return 0;
}

VA(0x00566c20, 0x15F)  // dc 0x128468
int SkeletonWindow::sacrifice(Message& msg)
{
    if (msg.m_codeX == Widget::WIDGET_RIGHT_SELECT
        && (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        normalDialog(
            g_transformerWindowHelp[TRANSFORMER_HELP_SACRIFICE].m_rclick,
            4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_codeX == Widget::WIDGET_DESELECT
        && !(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)) {
        SkeletonWindow* window =
            static_cast<SkeletonWindow*>(msg.m_window);
        for (long i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
            CreatureType type = window->m_armies[1]->m_armyTypes[i];
            if (type == CREATURE_NONE || type == g_deathCreature[type])
                continue;

            sprintf(g_text,
                    DATA_COMPGEN(0x006609e0, transformerKillSampleFormat,
                                 "%skill.82M"),
                    g_creatureTypeTraits[type].m_samplePrefix);
            Sample* newSample = ResourceManager::getSample(g_text);
            window->m_deathSamples.push_back(newSample);
            g_soundManager->memorySample(newSample);
            window->m_armies[1]->m_armyTypes[i] = g_deathCreature[type];
            window->update(1, i);
        }
        window->updateButtons();
        window->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        return MESSAGE_DISPATCH_CONSUME;
    }
    return 0;
}

#if 0  // @carcass: final duplicate STLport helper rows

// ..\stlport\stl_construct.h:53
DC_ONLY(0x129b30, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x129b34, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x129b38, 0x4)
void std::__destroy_aux()
{
    // @stub
}

#endif  // @carcass
