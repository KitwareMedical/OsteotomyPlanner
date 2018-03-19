/*==============================================================================

Program: 3D Slicer

Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

==============================================================================*/

#include "ButtonItemDelegate.h"
#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <iostream>

ButtonItemDelegate::ButtonItemDelegate(QObject *parent, const QPixmap &closeIcon)
    : QStyledItemDelegate(parent)
    , m_buttonIcon(closeIcon)
{
    if (m_buttonIcon.isNull())
    {
        m_buttonIcon = qApp->style()
            ->standardPixmap(QStyle::SP_DialogCloseButton);
    }
}

QPoint ButtonItemDelegate::buttonPos(const QStyleOptionViewItem &option) const
{
    return QPoint(option.rect.right() - m_buttonIcon.width() - margin,
        option.rect.center().y() - m_buttonIcon.height() / 2);
}

void ButtonItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);
    // Only display button for child items (as flagged by model)
    if (index.flags() & Qt::ItemIsUserCheckable)
    {
        painter->drawPixmap(buttonPos(option), m_buttonIcon);
    }
}

QSize ButtonItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);

    // Make some room for the icon
    if (index.flags() & Qt::ItemIsUserCheckable) {
        size.rwidth() += m_buttonIcon.width() + margin * 2;
        size.setHeight(qMax(size.height(),
            m_buttonIcon.height() + margin * 2));
    }
    return size;
}

bool ButtonItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    // Emit a signal when the icon is clicked
    if (index.flags() & Qt::ItemIsUserCheckable &&
        event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QRect buttonRect = m_buttonIcon.rect()
            .translated(buttonPos(option));
        
        if (buttonRect.contains(mouseEvent->pos()))
        {
          emit buttonIndexClicked(index);
        }
    }
    return false;
}
