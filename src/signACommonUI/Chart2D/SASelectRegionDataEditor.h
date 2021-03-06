#ifndef SASELECTREGIONDATAEDITOR_H
#define SASELECTREGIONDATAEDITOR_H
#include "SACommonUIGlobal.h"
#include "SASelectRegionEditor.h"
class SASelectRegionDataEditorPrivate;
class SA_COMMON_UI_EXPORT SASelectRegionDataEditor : public SAAbstractPlotEditor
{
    Q_OBJECT
    SA_IMPL(SASelectRegionDataEditor)
public:
    SASelectRegionDataEditor(SAChart2D *parent);
    ~SASelectRegionDataEditor();
    enum RTTI
    {
        RTTISelectDataRegionEditor = (SAAbstractPlotEditor::RTTIUserDefine + 14)
    };
    virtual int rtti() const;
    //
    const SAChart2D* chart2D() const;
    SAChart2D* chart2D();
private slots:
    void onCurrentSelectItemsChanged(const QList<QwtPlotItem*>& items);
private:
    //更新选中的索引
    void updateRegionIndex();
protected:
    virtual bool mousePressEvent(const QMouseEvent *e);
    virtual bool mouseMovedEvent( const QMouseEvent *e);
    virtual bool mouseReleasedEvent( const QMouseEvent *e);
    virtual bool keyPressEvent(const QKeyEvent *e);
    virtual bool keyReleaseEvent(const QKeyEvent *e);
};

#endif // SASELECTREGIONDATAEDITOR_H
