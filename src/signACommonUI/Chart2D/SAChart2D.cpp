#include "SAChart2D.h"
#include "SAChart.h"
#include "SAAbstractDatas.h"
#include "SARandColorMaker.h"
#include "SAFigureGlobalConfig.h"
#include <memory>
#include "SAXYSeries.h"
#include "SABarSeries.h"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>
#include "SAValueManagerMimeData.h"
#include "SAValueManager.h"
#include "SATendencyChartDataSelectDialog.h"
#include "SALog.h"
#include "SARectRegionSelectEditor.h"
#include "SAEllipseRegionSelectEditor.h"
#include "SAPolygonRegionSelectEditor.h"
#include <QUndoStack>
#include "SAFigureOptCommands.h"
#include "qwt_text.h"
#include "SASelectRegionShapeItem.h"
#include <numeric>
#include "SAScatterSeries.h"
#include "SABoxSeries.h"
#include "SAHistogramSeries.h"
class SAChart2DPrivate
{
    SA_IMPL_PUBLIC(SAChart2D)
public:
    QUndoStack m_undoStack;
    SAChart2D::SelectionMode m_selectMode;///< 选择模式
    SAAbstractRegionSelectEditor* m_chartSelectRigionEditor;///< 矩形选择编辑器
    QMap<QString,QPainterPath> m_selectionMap;///< 此dict保存的选区，选区可以保存，并加载
    SASelectRegionShapeItem* m_seclectRegionItem;///< 用于显示选区的item
    SAAbstractPlotEditor* m_editor;///< 额外的编辑器
    QList<QwtPlotItem*> m_currentSelectItem;///<当前选择的条目
    bool m_isSpaceLongPressed;///<记录空格长按
    bool m_isEnablePannerBeforePressedSpace;///< 记录按下空格前是否处于panner状态
    bool m_isEnableZoomBeforePressedSpace;///< 记录按下空格前是否处于zoom状态
public:
    SAChart2DPrivate(SAChart2D* p):q_ptr(p)
      ,m_selectMode(SAChart2D::NoneSelection)
      ,m_chartSelectRigionEditor(nullptr)
      ,m_seclectRegionItem(nullptr)
      ,m_editor(nullptr)
      ,m_isSpaceLongPressed(false)
      ,m_isEnablePannerBeforePressedSpace(false)
      ,m_isEnableZoomBeforePressedSpace(false)
    {

    }
    void setSpaceLongPressed()
    {
        if(!m_isSpaceLongPressed)
        {
            //说明第一次进入
            q_ptr->setCursor(Qt::OpenHandCursor);
            m_isEnablePannerBeforePressedSpace = q_ptr->isEnablePanner();
            m_isEnableZoomBeforePressedSpace = q_ptr->isEnableZoomer();
            q_ptr->enablePanner(true);
        }
        m_isSpaceLongPressed = true;
    }
    void setSpaceRelease()
    {
        if(m_isSpaceLongPressed)
        {
            //说明之前处于长按状态
            q_ptr->unsetCursor();
            if(m_isEnablePannerBeforePressedSpace)
                q_ptr->enablePanner(true);
            if(m_isEnableZoomBeforePressedSpace)
                q_ptr->enableZoomer(true);
        }
        m_isSpaceLongPressed = false;
    }

    void appendItemAddCommand(QwtPlotItem* item,const QString& des)
    {
        m_undoStack.push(new SAFigureChartItemAddCommand(q_ptr,item,des));
    }
    void appendItemDeleteCommand(QwtPlotItem* item,const QString& des)
    {
        m_undoStack.push(new SAFigureChartItemDeleteCommand(q_ptr,item,des));
    }
    void appendItemListAddCommand(QList<QwtPlotItem*> itemList,const QString& des)
    {
        m_undoStack.push(new SAFigureChartItemListAddCommand(q_ptr,itemList,des));
    }
    void appendRemoveItemCommand(QwtPlotItem* item,const QString& des)
    {
        m_undoStack.push(new SAFigureChartItemDeleteCommand(q_ptr,item,des));
    }
    void appendAddSelectionRegionCommand(const QPainterPath& newRegion,const QString& des)
    {
        m_undoStack.push(new SAFigureChartSelectionRegionAddCommand(q_ptr,newRegion,des));
    }
    void appendRemoveCurveDataInRangCommand(const QList<QwtPlotCurve *>& curves,const QString& des)
    {
        m_undoStack.push(new SAFigureRemoveCurveDataInRangCommand(q_ptr,curves,des));
    }


    void createRegionShapeItem()
    {
        if(nullptr == m_seclectRegionItem)
        {
            m_seclectRegionItem = new SASelectRegionShapeItem("region select item");
            m_seclectRegionItem->setZ(std::numeric_limits<double>::max());
            m_seclectRegionItem->attach(q_ptr);
        }
    }
    void releaseRegionShapeItem()
    {
        if(nullptr != m_seclectRegionItem)
        {
            m_seclectRegionItem->detach();
            delete m_seclectRegionItem;
            m_seclectRegionItem = nullptr;
        }
    }
};

SAChart2D::SAChart2D(QWidget *parent):SA2DGraph(parent)
  ,d_ptr(new SAChart2DPrivate(this))
{
    setAcceptDrops(true);
}

SAChart2D::~SAChart2D()
{
}

void SAChart2D::addItem(QwtPlotItem *item, const QString &des)
{
    d_ptr->appendItemAddCommand(item,des);
}

QwtPlotCurve *SAChart2D::addCurve(const double *xData, const double *yData, int size)
{
    if(size<=0)
    {
        return nullptr;
    }
    QwtPlotCurve* pCurve = nullptr;
    pCurve = new QwtPlotCurve;
    pCurve->setYAxis(yLeft);
    pCurve->setXAxis(xBottom);
    pCurve->setStyle(QwtPlotCurve::Lines);
    pCurve->setSamples(xData,yData,size);
    addItem(pCurve,tr("add curve:%1").arg(pCurve->title().text()));
    return pCurve;
}

QwtPlotCurve *SAChart2D::addCurve(const QVector<QPointF> &xyDatas)
{
    QwtPlotCurve* pCurve = nullptr;
    pCurve = new QwtPlotCurve;
    pCurve->setYAxis(yLeft);
    pCurve->setXAxis(xBottom);
    pCurve->setStyle(QwtPlotCurve::Lines);
    pCurve->setSamples(xyDatas);
    addItem(pCurve,tr("add curve:%1").arg(pCurve->title().text()));
    return pCurve;
}

QwtPlotCurve *SAChart2D::addCurve(const QVector<double> &xData, const QVector<double> &yData)
{
    QwtPlotCurve* pCurve = nullptr;
    pCurve = new QwtPlotCurve;
    pCurve->setYAxis(yLeft);
    pCurve->setXAxis(xBottom);
    pCurve->setStyle(QwtPlotCurve::Lines);
    pCurve->setSamples(xData,yData);
    addItem(pCurve,tr("add curve:%1").arg(pCurve->title().text()));
    return pCurve;
}

///
/// \brief 添加曲线
/// \param datas
/// \return
///
SAXYSeries *SAChart2D::addCurve(SAAbstractDatas *datas)
{
    QScopedPointer<SAXYSeries> series(new SAXYSeries(datas->getName(),datas));
    if(series->dataSize() <= 0)
    {
        return nullptr;
    }

    series->setPen(SARandColorMaker::getCurveColor()
                ,SAFigureGlobalConfig::getPlotCurWidth(series->dataSize()));
    addItem(series.data(),tr("add curve:%1").arg(series->title().text()));
    return series.take();
}
///
/// \brief SAChart2D::addCurve
/// \param datas
/// \param xStart
/// \param xDetal
/// \return
///
SAXYSeries *SAChart2D::addCurve(SAAbstractDatas *datas, double xStart, double xDetal)
{
    QScopedPointer<SAXYSeries> series(new SAXYSeries(datas->getName()));
    series->setSamples(datas,xStart,xDetal);
    if(series->dataSize() <= 0)
    {
        return nullptr;
    }
    series->setPen(SARandColorMaker::getCurveColor()
                ,SAFigureGlobalConfig::getPlotCurWidth(series->dataSize()));
    addItem(series.data(),tr("add curve:%1").arg(series->title().text()));
    return series.take();
}
///
/// \brief SAChart2D::addCurve
/// \param x
/// \param y
/// \param name
/// \return
///
SAXYSeries *SAChart2D::addCurve(SAAbstractDatas *x, SAAbstractDatas *y, const QString &name)
{
    QScopedPointer<SAXYSeries> series(new SAXYSeries(name));
    series->setSamples(x,y);
    if(series->dataSize() <= 0)
    {
        return nullptr;
    }
    series->setPen(SARandColorMaker::getCurveColor()
                ,SAFigureGlobalConfig::getPlotCurWidth(series->dataSize()));
    addItem(series.data(),tr("add curve:%1").arg(series->title().text()));
    return series.take();
}





///
/// \brief SAChart2D::addBar
/// \param datas
/// \return
///
SAHistogramSeries *SAChart2D::addHistogram(SAAbstractDatas *datas)
{
    QScopedPointer<SAHistogramSeries> series(new SAHistogramSeries(datas,datas->getName()));
    if(series->dataSize() <= 0)
    {
        return nullptr;
    }
    QColor clr = SARandColorMaker::getCurveColor();
    series->setBrush(QBrush(clr));
    series->setPen(clr,SAFigureGlobalConfig::getPlotCurWidth(series->dataSize()));
    series->setStyle(QwtPlotHistogram::Columns);
    addItem(series.data(),tr("add Histogram:%1").arg(series->title().text()));
    return series.take();
}


SABarSeries *SAChart2D::addBar(SAAbstractDatas *datas)
{
    QScopedPointer<SABarSeries> series(new SABarSeries(datas,datas->getName()));
    if(series->dataSize() <= 0)
    {
        return nullptr;
    }
    QColor clr = SARandColorMaker::getCurveColor();
    QwtColumnSymbol *symbol = new QwtColumnSymbol( QwtColumnSymbol::Box );
    symbol->setPalette(clr);
    series->setSymbol(symbol);
    addItem(series.data(),tr("add bar:%1").arg(series->title().text()));
    return series.take();
}


///
/// \brief 绘制散点图-支持redo/undo
/// \param datas
/// \return
///
SAScatterSeries *SAChart2D::addScatter(SAAbstractDatas *datas)
{
    QScopedPointer<SAScatterSeries> series(new SAScatterSeries(datas->getName(),datas));
    if(series->dataSize() <= 0)
    {
        return nullptr;
    }
    QColor clr = SARandColorMaker::getCurveColor();
    QPen pen = series->pen();
    pen.setColor(clr);
    series->setPen(pen);
    addItem(series.data(),tr("add scatter:%1").arg(series->title().text()));
    return series.take();
}


///
/// \brief 绘制箱盒图
/// \param datas
/// \return
///
SABoxSeries *SAChart2D::addBox(SAAbstractDatas *datas)
{
    QScopedPointer<SABoxSeries> series(new SABoxSeries(datas,datas->getName()));
//    if(series->dataSize() <= 0)
//    {
//        return nullptr;
//    }
    addItem(series.data(),tr("add box:%1").arg(series->title().text()));
    return series.take();
}


///
/// \brief 添加一条竖直线
/// \return
///
QwtPlotMarker *SAChart2D::addVLine(double val)
{
    QwtPlotMarker *marker = new QwtPlotMarker();
    marker->setXValue(val);
    marker->setLineStyle( QwtPlotMarker::VLine );
    marker->setItemAttribute( QwtPlotItem::Legend, true );
    addItem(marker,tr("add VLine %1").arg(marker->title().text()));
    return marker;
}
///
/// \brief 添加一条水平线
/// \param val
/// \return
///
QwtPlotMarker *SAChart2D::addHLine(double val)
{
    QwtPlotMarker *marker = new QwtPlotMarker();
    marker->setYValue(val);
    marker->setLineStyle( QwtPlotMarker::HLine );
    marker->setItemAttribute( QwtPlotItem::Legend, true );
    addItem(marker,tr("add HLine %1").arg(marker->title().text()));
    return marker;
}

///
/// \brief 移除一个对象
/// \param item
///
void SAChart2D::removeItem(QwtPlotItem *item)
{
    d_ptr->appendRemoveItemCommand(item,tr("delete [%1]").arg(item->title().text()));
}

///
/// \brief 移除范围内数据
/// \param curves 需要移除的曲线列表
///
void SAChart2D::removeDataInRang(const QList<QwtPlotCurve *>& curves)
{
    QPainterPath region = getSelectionRange();
    if(region.isEmpty())
    {
        return;
    }
    SAAbstractRegionSelectEditor* editor = getRegionSelectEditor();
    if(nullptr == editor)
    {
        return;
    }
    d_ptr->appendRemoveCurveDataInRangCommand(curves,tr("remove rang data"));
#if 0
    QHash<QPair<int,int>,QPainterPath> otherScaleMap;
    for(int i=0;i<curves.size();++i)
    {
        int xa = curves[i]->xAxis();
        int ya = curves[i]->yAxis();
        if(xa == editor->getXAxis() && ya == editor->getYAxis())
        {
            SAChart::removeDataInRang(region,curves[i]);
        }
        else
        {
            QPair<int,int> axiss=qMakePair(xa,ya);
            if(!otherScaleMap.contains(axiss))
            {
                otherScaleMap[axiss] = editor->transformToOtherAxis(xa,ya);

            }
            SAChart::removeDataInRang(otherScaleMap.value(axiss)
                                      ,curves[i]);
        }
    }
    setAutoReplot(true);
    replot();
#endif
}

void SAChart2D::removeDataInRang()
{
    removeDataInRang(getCurrentSelectPlotCurveItems());
}
///
/// \brief 获取选择范围内的数据,如果当前没有选区，返回false
/// \param xy
/// \param cur
/// \return
///
bool SAChart2D::getDataInSelectRange(QVector<QPointF> &xy, QwtPlotCurve *cur)
{
    QPainterPath region = getSelectionRange();
    if(region.isEmpty())
    {
        return false;
    }
    SAAbstractRegionSelectEditor* editor = getRegionSelectEditor();
    if(nullptr == editor)
    {
        return false;
    }
    int xa = cur->xAxis();
    int ya = cur->yAxis();
    if(xa == editor->getXAxis() && ya == editor->getYAxis())
    {
        SAChart::getDataInRang(region,cur,xy);
    }
    else
    {
        region = editor->transformToOtherAxis(xa,ya);
        if(!region.isEmpty())
        {
            SAChart::getDataInRang(region,cur,xy);
        }
    }
    return true;
}
///
/// \brief 开始选择模式
/// 选择模式可分为矩形，圆形等，具体见\sa SelectionMode
/// \param mode 选择模式
/// \see SelectionMode
///
void SAChart2D::enableSelection(SelectionMode mode,bool on)
{
    SA_D(SAChart2D);
    if(on)
    {
        if(NoneSelection == mode)
        {
            return;
        }
        stopSelectMode();
        d->m_selectMode = mode;
        switch(mode)
        {
        case RectSelection:startRectSelectMode();break;
        case EllipseSelection:startEllipseSelectMode();break;
        case PolygonSelection:startPolygonSelectMode();break;
        default:return;
        }
    }
    else
    {
        stopSelectMode();
    }
}
///
/// \brief 判断当前的选择模式
/// \param mode
/// \return
///
bool SAChart2D::isEnableSelection(SAChart2D::SelectionMode mode) const
{
    SA_DC(SAChart2D);
    if(nullptr == d->m_chartSelectRigionEditor)
    {
        return false;
    }
    if(!d->m_chartSelectRigionEditor->isEnabled())
    {
        return false;
    }
    switch(mode)
    {
    case RectSelection:
        return d->m_chartSelectRigionEditor->rtti() == SAAbstractPlotEditor::RTTIRectRegionSelectEditor;
    case EllipseSelection:
        return d->m_chartSelectRigionEditor->rtti() == SAAbstractPlotEditor::RTTIEllipseRegionSelectEditor;
    case PolygonSelection:
        return d->m_chartSelectRigionEditor->rtti() == SAAbstractPlotEditor::RTTIPolygonRegionSelectEditor;
    default:
        break;
    }
    return false;
}
///
/// \brief 结束当前的选择模式
///
void SAChart2D::stopSelectMode()
{
    SA_D(SAChart2D);
    if(nullptr == d->m_chartSelectRigionEditor)
    {
        return;
    }
    d->m_chartSelectRigionEditor->setEnabled(false);
    d->m_selectMode = NoneSelection;
}
///
/// \brief 选区选择完成
/// \param shape
///
void SAChart2D::onSelectionFinished(const QPainterPath &shape)
{
    d_ptr->appendAddSelectionRegionCommand(shape,tr("add selection region"));
}
///
/// \brief 清除所有选区
///
void SAChart2D::clearAllSelectedRegion()
{
    SA_D(SAChart2D);
    if(nullptr == d->m_chartSelectRigionEditor)
    {
        return;
    }
    delete d->m_chartSelectRigionEditor;
    d->m_chartSelectRigionEditor = nullptr;
    d->releaseRegionShapeItem();
}
///
/// \brief 判断是否有选区
/// \return
///
bool SAChart2D::isRegionVisible() const
{
    SA_DC(SAChart2D);
    if(nullptr == d->m_seclectRegionItem)
    {
        return false;
    }
    return d->m_seclectRegionItem->isVisible();
}
///
/// \brief 获取当前正在显示的选择区域
/// \return
///
SAChart2D::SelectionMode SAChart2D::currentSelectRegionMode() const
{
    SA_DC(SAChart2D);
    return d->m_selectMode;
}
///
/// \brief 获取矩形选择编辑器
/// \return 如果没有设置编辑器，返回nullptr
///
SAAbstractRegionSelectEditor *SAChart2D::getRegionSelectEditor()
{
    SA_D(SAChart2D);
    return d->m_chartSelectRigionEditor;
}

const SAAbstractRegionSelectEditor *SAChart2D::getRegionSelectEditor() const
{
    SA_DC(SAChart2D);
    return d->m_chartSelectRigionEditor;
}
///
/// \brief 获取当前可见的选区的范围
/// \return
///
QPainterPath SAChart2D::getSelectionRange() const
{
    SA_DC(SAChart2D);
    if(!d->m_seclectRegionItem)
    {
        return QPainterPath();
    }
    return d->m_seclectRegionItem->shape();
}
///
/// \brief 获取当前可见的选区的范围,选区范围更加坐标轴进行映射
/// \param xaxis
/// \param yaxis
/// \return
///
QPainterPath SAChart2D::getSelectionRange(int xaxis, int yaxis) const
{
    QPainterPath shape = getSelectionRange();

    QwtScaleMap xRawMap = canvasMap(axisEnabled(QwtPlot::yLeft) ? QwtPlot::yLeft : QwtPlot::yRight);
    QwtScaleMap yRawMap = canvasMap(axisEnabled(QwtPlot::xBottom) ? QwtPlot::xBottom : QwtPlot::xTop);
    const SAAbstractRegionSelectEditor* editor = getRegionSelectEditor();
    if(editor)
    {
        if(xaxis == editor->getXAxis() && yaxis == editor->getYAxis())
        {
            return shape;
        }
        return editor->transformToOtherAxis(xaxis,yaxis);
    }
    QwtScaleMap xMap = canvasMap( xaxis );
    QwtScaleMap yMap = canvasMap( yaxis );

    const int eleCount = shape.elementCount();
    for(int i=0;i<eleCount;++i)
    {
        const QPainterPath::Element &el = shape.elementAt( i );
        QPointF tmp = QwtScaleMap::transform(xRawMap,yRawMap,QPointF(el.x,el.y));
        tmp = QwtScaleMap::invTransform(xMap,yMap,tmp);
        shape.setElementPositionAt( i, tmp.x(), tmp.y() );
    }
    return shape;
}

void SAChart2D::setSelectionRange(const QPainterPath &painterPath)
{
    SA_D(SAChart2D);
    if(!d->m_seclectRegionItem)
    {
        d->createRegionShapeItem();
    }
    d->m_seclectRegionItem->setShape(painterPath);
    if(!d->m_seclectRegionItem->isVisible())
    {
        d->m_seclectRegionItem->setVisible(true);
    }
    emit selectionRegionChanged(painterPath);
}
///
/// \brief 判断当前的条目的x，y轴关联是否和选区的一致
/// \param item
/// \return 如果没有选区，或者不一致，返回false
///
bool SAChart2D::isSelectionRangeAxisMatch(const QwtPlotItem *item)
{
    SAAbstractRegionSelectEditor* editor = getRegionSelectEditor();
    if(!editor)
    {
        return false;
    }
    return (editor->getXAxis() == item->xAxis()) && (editor->getYAxis() == item->yAxis());
}
///
/// \brief redo
///
void SAChart2D::redo()
{
    d_ptr->m_undoStack.redo();
}
///
/// \brief undo
///
void SAChart2D::undo()
{
    d_ptr->m_undoStack.undo();
}

void SAChart2D::appendCommand(SAFigureOptCommand *cmd)
{
    d_ptr->m_undoStack.push(cmd);
}
///
/// \brief 设置一个编辑器，编辑器的内存交由SAChart2D管理，SAChart2D只能存在一个额外的编辑器
///
/// \param editor 传入null将解除之前的editor
///
void SAChart2D::setEditor(SAAbstractPlotEditor *editor)
{
    if(d_ptr->m_editor)
    {
        delete (d_ptr->m_editor);
    }
    d_ptr->m_editor = editor;
}
///
/// \brief 获取当前的编辑器
/// \return
///
SAAbstractPlotEditor *SAChart2D::getEditor() const
{
    return d_ptr->m_editor;
}
///
/// \brief 当前选择的条目
/// \return
///
QList<QwtPlotItem *> SAChart2D::getCurrentSelectItems() const
{
    SA_DC(SAChart2D);
    return d->m_currentSelectItem;
}

QList<QwtPlotCurve *> SAChart2D::getCurrentSelectPlotCurveItems() const
{
    SA_DC(SAChart2D);
    const QList<QwtPlotItem *>& items = getCurrentSelectItems();
    QList<QwtPlotCurve *> res;
    for(int i=0;i<items.size();++i)
    {
        if(QwtPlotItem::Rtti_PlotCurve == items[i]->rtti())
        {
            res.append(static_cast<QwtPlotCurve *>(items[i]));
        }
    }
    return res;
}

void SAChart2D::setCurrentSelectItems(const QList<QwtPlotItem *> &items)
{
    SA_D(SAChart2D);
    d->m_currentSelectItem = items;
    emit currentSelectItemsChanged(items);
}

void SAChart2D::setCurrentSelectPlotCurveItems(const QList<QwtPlotCurve *> &items)
{
    SA_D(SAChart2D);
    d->m_currentSelectItem.clear();
    for(int i=0;i<items.size();++i)
    {
        d->m_currentSelectItem.append(items[i]);
    }
    emit currentSelectItemsChanged(d->m_currentSelectItem);
}
///
/// \brief 把当前存在的编辑器禁止
///
void SAChart2D::unenableEditor()
{
    if(d_ptr->m_chartSelectRigionEditor)
    {
        d_ptr->m_chartSelectRigionEditor->setEnabled(false);
    }
    if(SAAbstractPlotEditor* editor = getEditor())
    {
        editor->setEnabled(false);
    }
    SA2DGraph::unenableEditor();
}

///
/// \brief 开始矩形选框模式
///
void SAChart2D::startRectSelectMode()
{
    SA_D(SAChart2D);
    if(d->m_chartSelectRigionEditor)
    {
        delete d->m_chartSelectRigionEditor;
        d->m_chartSelectRigionEditor = nullptr;
    }
    if(nullptr == d->m_chartSelectRigionEditor)
    {
        d->m_chartSelectRigionEditor = new SARectRegionSelectEditor(this);
        connect(d->m_chartSelectRigionEditor,&SAAbstractRegionSelectEditor::finishSelection
                ,this,&SAChart2D::onSelectionFinished);
        d->m_chartSelectRigionEditor->setSelectRegion(getSelectionRange());
    }
    d->m_chartSelectRigionEditor->setEnabled(true);
}
///
/// \brief 开始椭圆选框模式
///
void SAChart2D::startEllipseSelectMode()
{
    SA_D(SAChart2D);
    if(d->m_chartSelectRigionEditor)
    {
        delete d->m_chartSelectRigionEditor;
        d->m_chartSelectRigionEditor = nullptr;
    }
    if(nullptr == d->m_chartSelectRigionEditor)
    {
        d->m_chartSelectRigionEditor = new SAEllipseRegionSelectEditor(this);
        connect(d->m_chartSelectRigionEditor,&SAAbstractRegionSelectEditor::finishSelection
                ,this,&SAChart2D::onSelectionFinished);
        d->m_chartSelectRigionEditor->setSelectRegion(getSelectionRange());
    }
    d->m_chartSelectRigionEditor->setEnabled(true);
}
///
/// \brief 开始多边形选框模式
///
void SAChart2D::startPolygonSelectMode()
{
    SA_D(SAChart2D);
    if(d->m_chartSelectRigionEditor)
    {
        delete d->m_chartSelectRigionEditor;
        d->m_chartSelectRigionEditor = nullptr;
    }
    if(nullptr == d->m_chartSelectRigionEditor)
    {
        d->m_chartSelectRigionEditor = new SAPolygonRegionSelectEditor(this);
        connect(d->m_chartSelectRigionEditor,&SAAbstractRegionSelectEditor::finishSelection
                ,this,&SAChart2D::onSelectionFinished);
        d->m_chartSelectRigionEditor->setSelectRegion(getSelectionRange());
    }
    d->m_chartSelectRigionEditor->setEnabled(true);
}

///
/// \brief 向chart添加一组数据
/// \param datas
///
QList<SAXYSeries *> SAChart2D::addDatas(const QList<SAAbstractDatas *> &datas)
{
    saPrint();
    QList<SAXYSeries *> res;
    const int dataCount = datas.size();
    if(1 == dataCount)
    {
        if(SA::Dim1 == datas[0]->getDim())
        {
            //1维向量
            SATendencyChartDataSelectDialog dlg(this);
            if(QDialog::Accepted != dlg.exec())
            {
                return QList<SAXYSeries *>();
            }

            QList<QwtPlotItem*> itemList;
            if(dlg.isFollow())
            {
                QwtPlotCurve* cur = dlg.getSelFollowCurs();
                std::unique_ptr<SAXYSeries> series(new SAXYSeries(datas[0]->getName()));
                QVector<double> x,y;
                if(!SAAbstractDatas::converToDoubleVector(datas[0],y))
                {
                    return QList<SAXYSeries *>();
                }
                SAChart::getXDatas(x,cur);
                if(x.size() < y.size())
                {
                    y.resize(x.size());
                }
                else
                {
                    x.resize(y.size());
                }
                series->setSamples(x,y);
                series->setPen(SARandColorMaker::getCurveColor()
                            ,SAFigureGlobalConfig::getPlotCurWidth(series->dataSize()));
                itemList.append(series.release());
            }
            else if(dlg.isSelDef())
            {
                double startData,addedData;
                dlg.getSelDefData(startData,addedData);
                SAXYSeries* seris = addCurve(datas[0],startData,addedData);
                res.append(seris);
            }

            if(itemList.size() > 0)
            {
                d_ptr->appendItemListAddCommand(itemList,tr("add datas"));
                for(int i=0;i<itemList.size();++i)
                {
                    res.append(static_cast<SAXYSeries*>(itemList[i]));
                }
            }
            return res;
        }
        else
        {
           SAXYSeries* seris = addCurve(datas[0]);
           if(seris)
           {
                res.append(seris);
           }
        }
    }
    else if (2 == dataCount)
    {
        if(SA::Dim1 == datas[0]->getDim()
                &&
                SA::Dim1 == datas[1]->getDim())
        {
            SAXYSeries* seris = addCurve(datas[0],datas[1]
                    ,QString("%1-%2")
                    .arg(datas[0]->getName())
                    .arg(datas[1]->getName()));
            res.append(seris);
        }
    }
    else
    {
        setAutoReplot(false);
        std::for_each(datas.cbegin(),datas.cend(),[this,&res](SAAbstractDatas* data){
            SAXYSeries* seris = addCurve(data);
            if(seris)
            {
                res.append(seris);
            }
        });
        setAutoReplot(true);
        replot();
    }
    return res;
}

void SAChart2D::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasFormat(SAValueManagerMimeData::valueIDMimeType()))
    {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void SAChart2D::dragMoveEvent(QDragMoveEvent *event)
{
    if(event->mimeData()->hasFormat(SAValueManagerMimeData::valueIDMimeType()))
    {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void SAChart2D::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasFormat(SAValueManagerMimeData::valueIDMimeType()))
    {
        QList<int> ids;
        if(SAValueManagerMimeData::getValueIDsFromMimeData(event->mimeData(),ids))
        {
            QList<SAAbstractDatas*> datas = saValueManager->findDatas(ids);
            addDatas(datas);
        }
    }
}




void SAChart2D::keyPressEvent(QKeyEvent *e)
{
    if(e)
    {
        if(e->isAutoRepeat() && Qt::Key_Space == e->key())
        {
            //记录空格长按
            d_ptr->setSpaceLongPressed();
        }
    }
    return SA2DGraph::keyPressEvent(e);
}

void SAChart2D::keyReleaseEvent(QKeyEvent *e)
{
    if(e)
    {
        //qDebug() << "keyReleaseEvent isAutoRepeat"<<e->isAutoRepeat()<<" key:"<<e->key();
        if(d_ptr->m_isSpaceLongPressed)
        {
            if(!e->isAutoRepeat() && Qt::Key_Space == e->key())
            {
                d_ptr->setSpaceRelease();
            }
        }
    }
    return SA2DGraph::keyReleaseEvent(e);
}


