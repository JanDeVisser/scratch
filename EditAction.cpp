/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <Document.h>

namespace Scratch {

void EditAction::undo(Document& doc) const
{
    switch (m_type) {
    case EditActionType::InsertText:
        doc.erase(cursor(), static_cast<int>(text().length()));
        break;
    case EditActionType::DeleteText:
        doc.insert_text(text(), cursor());
        break;
    case EditActionType::CursorMove:
        doc.set_point_and_mark(cursor());
        break;
    }
}

void EditAction::redo(Document& doc) const
{
    switch (m_type) {
    case EditActionType::InsertText:
        doc.insert_text(text(), cursor());
        break;
    case EditActionType::DeleteText:
        doc.erase(cursor(), static_cast<int>(text().length()));
        break;
    case EditActionType::CursorMove:
        doc.set_point_and_mark(pointer());
        break;
    }
}

std::optional<EditAction> EditAction::merge(EditAction const& merge_with) const
{
    if (merge_with.type() != type())
        return {};
    switch (type()) {
    case EditActionType::InsertText:
        if (merge_with.cursor() == cursor() + static_cast<int>(text().length()))
            return EditAction { EditActionType::InsertText, cursor(), text() + merge_with.text() };
        break;
    case EditActionType::DeleteText:
        if (cursor() == merge_with.cursor() + static_cast<int>(merge_with.text().length()))
            return EditAction { EditActionType::DeleteText, merge_with.cursor(), merge_with.text() + text() };
        break;
    case EditActionType::CursorMove:
        if (pointer() == merge_with.cursor())
            return EditAction { EditActionType::CursorMove, cursor(), merge_with.pointer() };
        break;
    }
    return {};
}

EditAction EditAction::insert_text(int cursor, std::string text)
{
    return { EditActionType::InsertText, cursor, std::move(text) };
}

EditAction EditAction::delete_text(int cursor, std::string text)
{
    return { EditActionType::DeleteText, cursor, std::move(text) };
}

EditAction EditAction::move_cursor(int from, int new_cursor)
{
    return { EditActionType::CursorMove, from, new_cursor };
}

EditAction::EditAction(EditActionType type, int cursor, std::string text)
    : m_type(type)
    , m_cursor(cursor)
    , m_text(std::move(text))
{
}

EditAction::EditAction(EditActionType type, int cursor, int pointer)
    : m_type(type)
    , m_cursor(cursor)
    , m_pointer(pointer)
{
}

}
