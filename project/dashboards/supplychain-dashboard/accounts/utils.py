from accounts.models import User, UserCompany, Company, Role

from typing import List

def get_user_roles(user: User, company: Company) -> List[str]:
    """
    Get the permissions for a user in a specific company.
    """
    if not user or not user.is_authenticated:
        return []

    try:
        role_names = user.user_roles.filter(role__company=company).values_list('role__name', flat=True)
    except Exception as e:
        print(e)
        return []

    return role_names

