// ArmyDlg.h - the seven creature stack editor shared by the hero, town and
// guardian pages (Loki ArmyDlg.cpp). The glade widgets are named by a
// prefix (the owning page's "hp_", "tp_" or "gd_") plus the slot widget
// name; the signal handlers are looked up by the same prefixed names with
// dlsym. Layout from the image: TArmyDlgClient (privately inherited, its
// vtable pointer first), the client, the caller's army, the modified flag,
// the occupied stack count, _m_aStackData, the edited army (_m_army), the
// enabled flag and the prefix. Member names from the asserts:
// _m_aStackData with m_creatureType, m_quantity, m_typeCombo and
// m_typeItemData, _m_army, _s_kNumCreatureStacks; the others are not proven.
#ifndef HOMM3_EDITOR_ARMYDLG_H
#define HOMM3_EDITOR_ARMYDLG_H

#include "editor/stdafx.h"

#include <string>

#include "editor/Army.h"

// Told when the number of occupied stacks changes.
class TArmyDlgClient {
public:
    virtual void onNumOccupiedStacksChanged(unsigned int newNum, unsigned int oldNum) = 0;
};

class TArmyDlg : TArmyDlgClient {
public:
    TArmyDlg(const TArmy& army, char* prefix);
    TArmyDlg(TArmyDlgClient* pClient, const TArmy& army, char* prefix);

    unsigned int getNumOccupiedStacks() const { return _m_numOccupiedStacks; }
    const TArmy& getArmy() const { return _m_army; }
    bool wasModified() const { return _m_bModified; }

    virtual void OnOK();
    virtual void EnableWindow(bool bEnable) { OnEnable(bEnable); }
    virtual void onNumOccupiedStacksChanged(unsigned int newNum, unsigned int oldNum) {}
    BOOL IsWindowEnabled() { return _m_bEnabled; }
    virtual BOOL OnInitDialog();

    void OnEnable(bool bEnable);
    void OnSelChangeTypeCombo1();
    void OnSelChangeTypeCombo2();
    void OnSelChangeTypeCombo3();
    void OnSelChangeTypeCombo4();
    void OnSelChangeTypeCombo5();
    void OnSelChangeTypeCombo6();
    void OnSelChangeTypeCombo7();
    void OnKillFocusQtyEdit1();
    void OnKillFocusQtyEdit2();
    void OnKillFocusQtyEdit3();
    void OnKillFocusQtyEdit4();
    void OnKillFocusQtyEdit5();
    void OnKillFocusQtyEdit6();
    void OnKillFocusQtyEdit7();

    GtkWidget* army_widget(char* name);
    void* get_army_func(char* name);
    void disableArmyDlgSignals();
    void enableArmyDlgSignals();

private:
    static const int _s_kNumCreatureStacks = 7;

    enum { _s_kMaxComboItems = 1000 };

    struct TStackData {
        TCreatureType m_creatureType;
        unsigned int m_quantity;
        GtkCombo* m_typeCombo;
        int m_typeItemData[_s_kMaxComboItems];
        GtkEntry* m_quantityEdit;
        int m_quantityItemData[_s_kMaxComboItems];
        GtkSpinButton* m_quantitySpin;
    };

    void _retrieveStackQuantities();
    void _onSelChangeTypeCombo(unsigned int stackNum);
    void _onKillFocusQtyEdit(unsigned int stackNum);

    TArmyDlgClient* _m_pClient;
    const TArmy& _m_originalArmy;
    bool _m_bModified;
    unsigned int _m_numOccupiedStacks;
    TStackData _m_aStackData[_s_kNumCreatureStacks];
    TArmy _m_army;
    bool _m_bEnabled;
    string _m_prefix;
};

#endif  /* HOMM3_EDITOR_ARMYDLG_H */
