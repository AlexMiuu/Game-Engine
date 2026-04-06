#include "EditorState.hpp"

namespace gps {

    EditorState::EditorState()
        : m_mode(EditorMode::Play),
          m_activeTool(EditTool::Translate),
          m_isDragging(false),
          m_dragObjectID(-1),
          m_dragOffset(0.0f)
    {
    }

    EditorState::~EditorState() = default;

    void EditorState::ToggleMode() {
        m_mode = (m_mode == EditorMode::Play) ? EditorMode::Edit : EditorMode::Play;
        m_isDragging = false;
        m_dragObjectID = -1;
    }

    void EditorState::StartDrag(int objectID, const glm::vec3& objectPos, const glm::vec3& worldClickPos) {
        m_isDragging = true;
        m_dragObjectID = objectID;
        m_dragOffset = objectPos - worldClickPos;
    }

    void EditorState::EndDrag() {
        m_isDragging = false;
        m_dragObjectID = -1;
    }

} // namespace gps
