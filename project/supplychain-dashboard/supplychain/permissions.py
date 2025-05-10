from rest_framework import permissions
from abac.constants import Action


class ABACPermission(permissions.BasePermission):
    """
    Permission class enforcing ABAC policies using Action constants.
    """

    def has_permission(self, request, view):
        # Allow only authenticated users to access the API
        return bool(request.user and request.user.is_authenticated)

    def has_object_permission(self, request, view, obj):
        # Map DRF viewset actions to ABAC Action enums
        action_map = {
            'list':            Action.VIEW_PRODUCT,
            'retrieve':       Action.VIEW_PRODUCT,
            'create':         Action.CREATE_PRODUCT,
            'update':         Action.CHANGE_PRODUCT,
            'partial_update': Action.CHANGE_PRODUCT,
            'destroy':        Action.DELETE_PRODUCT,
        }

        action_enum = action_map.get(view.action)

        if not action_enum:
            return False

        # Use the Enum's value as the permission string
        return request.user.has_perm(action_enum.value, obj)
