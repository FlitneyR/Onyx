#include "Utils.h"

#include "imgui.h"
#include "Onyx/ECS/Entity.h"

ImGuiScopedID::ImGuiScopedID( const onyx::ecs::EntityID& id ) { ImGui::PushID( u32( id ) ); }
ImGuiScopedID::ImGuiScopedID( int id ) { ImGui::PushID( id ); }
ImGuiScopedID::ImGuiScopedID( const char* id ) { ImGui::PushID( id ); }
ImGuiScopedID::~ImGuiScopedID() { ImGui::PopID(); }

ImGuiScopedIndent::ImGuiScopedIndent( f32 amount ) { ImGui::Indent( m_amount = amount ); }
ImGuiScopedIndent::~ImGuiScopedIndent() { ImGui::Unindent( m_amount ); }
