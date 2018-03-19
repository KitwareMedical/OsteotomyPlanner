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

#ifndef __ButtonItemDelegate_h
#define __ButtonItemDelegate_h

#include <QStyledItemDelegate>

class ButtonItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:

    explicit ButtonItemDelegate(QObject *parent = 0, const QPixmap &closeIcon = QPixmap());
    

    QPoint buttonPos(const QStyleOptionViewItem &option) const;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    

    QSize sizeHint(const QStyleOptionViewItem &option,const QModelIndex &index) const;

    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);

signals:
    void buttonIndexClicked(const QModelIndex &);
private:
    QPixmap m_buttonIcon;
    static const int margin = 2; // pixels to keep arount the icon

    Q_DISABLE_COPY(ButtonItemDelegate)
};

#endif