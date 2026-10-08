// GUIGameObject.h - the editor's view of a map object (Loki
// GUIGameObject.cpp). Declared so far only as far as the window classes
// need it: TEditContext is the property-editing interface TMapView
// implements, one pure overload per object class, in the vtable order of
// __vt_8TMapView.Q214TGUIGameObject12TEditContext.
#ifndef HOMM3_EDITOR_GUIGAMEOBJECT_H
#define HOMM3_EDITOR_GUIGAMEOBJECT_H

class TGameObject;
class TNonRandomHero;
class TRandomHero;
class TPrison;
class TTown;
class TEvent;
class TMonster;
class TFlaggableObject;
class TAbandonedMine;
class TGarrison;
class TSign;
class TGameArtifact;
class TSpellScroll;
class TGameResource;
class TBlackBox;
class TScholar;
class TSeersHut;
class THolyGrail;
class TShrine;

class TGUIGameObject {
public:
    class TEditContext {
    public:
        virtual bool onEditProperties(TGameObject* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TNonRandomHero* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TRandomHero* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TPrison* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TTown* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TEvent* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TMonster* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TFlaggableObject* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TAbandonedMine* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TGarrison* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TSign* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TGameArtifact* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TSpellScroll* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TGameResource* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TBlackBox* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TScholar* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TSeersHut* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(THolyGrail* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TShrine* pObj, unsigned int objID) = 0;
    };
};

#endif  /* HOMM3_EDITOR_GUIGAMEOBJECT_H */
