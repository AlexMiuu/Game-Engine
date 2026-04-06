#pragma once
#include <glm/glm.hpp>

namespace gps {

    enum class EditorMode { Play, Edit };
    enum class EditTool { Translate, Scale };

    class EditorState {
    public:
        EditorState();
        ~EditorState();

        // Mode
        EditorMode GetMode() const { return m_mode; }
        void SetMode(EditorMode mode) { m_mode = mode; }
        void ToggleMode();
        bool IsEditMode() const { return m_mode == EditorMode::Edit; }
        bool IsPlayMode() const { return m_mode == EditorMode::Play; }

        // Active tool
        EditTool GetActiveTool() const { return m_activeTool; }
        void SetActiveTool(EditTool tool) { m_activeTool = tool; }

        // Drag state
        bool IsDragging() const { return m_isDragging; }
        void StartDrag(int objectID, const glm::vec3& objectPos, const glm::vec3& worldClickPos);
        void EndDrag();
        int GetDraggedObjectID() const { return m_dragObjectID; }
        glm::vec3 GetDragOffset() const { return m_dragOffset; }

    private:
        EditorMode m_mode;
        EditTool m_activeTool;

        bool m_isDragging;
        int m_dragObjectID;
        glm::vec3 m_dragOffset;
    };

} // namespace gps
