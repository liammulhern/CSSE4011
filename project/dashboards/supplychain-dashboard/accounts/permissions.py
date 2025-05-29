from rest_framework.permissions import BasePermission, SAFE_METHODS

from accounts.utils import get_user_roles

from accounts.models import Role

from supplychain.models import ProductOrder


class IsAuthenticatedOrValidQR(BasePermission):
    """
    Allow access if the user is JWT-authenticated,
    OR if they present a valid QR token matching this model+pk.
    """
    message = "Must be authenticated or provide a valid QR token for this product."

    def has_permission(self, request, view):
        # 1) JWT path
        if request.user and request.user.is_authenticated:
            return True

        # 2) QR-token path
        payload = getattr(request, "qr_payload", None)
        if not payload or payload.get("type") != "qr":
            return False

        model_name = view.get_queryset().model.__name__
        if payload.get("model") != model_name:
            return False

        try:
            token_id = int(payload.get("id"))
            url_id   = int(view.kwargs.get(view.lookup_field))
        except (TypeError, ValueError):
            return False

        return token_id == url_id

class IsCompanyAdminOrReadOnly(BasePermission):
    """
    - Any member (admin or viewer) may READ (GET, HEAD, OPTIONS)
      on objects owned by their company.
    - Only an ADMIN may CREATE / UPDATE / DELETE on those objects.
    """
    message = "You must be a company member to view, or an admin to modify."

    def has_object_permission(self, request, view, obj):

        print(f"Roles for user {request} {obj}")

        # If the object has an owner, check if the user is a member of that company
        if getattr(obj, "owner", None):
            roles = get_user_roles(request.user, obj.owner)

            if len(roles) > 0:
                # 2) safe methods allowed for all members
                if request.method in SAFE_METHODS:
                    return True

                # 3) only admins can write
                return Role.ADMIN in roles

        # If you are the owning company 
        if getattr(obj, "supplier", None):
            roles = get_user_roles(request.user, obj.supplier)

            if len(roles) > 0:
                # 2) safe methods allowed for all members
                if request.method in SAFE_METHODS:
                    return True

                # 3) only admins can write
                return Role.ADMIN in roles


        # If you aren’t in the owning company BUT you’re
        # in the assigned company and it’s a READ, allow:
        if getattr(obj, "receiver", None) and request.method in SAFE_METHODS:
            roles = get_user_roles(request.user, obj.receiver)

            return len(roles) > 0

        return False
