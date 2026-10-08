// Quest.h - what a Seer's Hut or a Quest Guard asks of a hero
// (C:\Dev\Heroes 3 Exp 2\Editor\Quest.cpp, by its RTTI). A quest takes a
// visitor (TQuest::TVisitor: the cloner, the writer, the equivalency
// testers and the quest dialogs); the defeat-hero and defeat-monster
// quests name their target by its link id.
//
// Ported so far: the declarations the map needs.
#ifndef HOMM3_EDITOR_QUEST_H
#define HOMM3_EDITOR_QUEST_H

// The quest base (RTTI TQuest: its vtable is the scalar deleting
// destructor, then the pure accept slot; a quest's accept, 0x495199,
// hands the visitor its own type).
class TQuest {
public:
    class TVisitor;

    virtual ~TQuest() {}
    virtual void accept(TVisitor& visitor) const = 0;
};

// A quest whose target is a linkable object: its third slot, pure here,
// yields the target's link id (both defeat quests share 0x4951cb). The map
// clears such quests when their target leaves (h3maped 0x4278a6).
class TLinkedQuest : public TQuest {
public:
    virtual unsigned int getLinkID() const = 0;
};

#endif  /* HOMM3_EDITOR_QUEST_H */
