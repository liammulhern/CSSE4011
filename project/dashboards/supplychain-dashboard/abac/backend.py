"""
A minimal Django auth backend that enforces ABAC policies.
"""

from django.contrib.auth.backends import BaseBackend
from django.contrib.contenttypes.models import ContentType
from abac.models import Policy


class ABACBackend(BaseBackend):
    """
    Checks user.has_perm('action', obj) by evaluating ABAC policies.
    """

    def has_perm(self, user_obj, perm, obj=None):
        """
        `perm` is the action name (e.g. 'view_event').
        `obj` is the model instance to check against.
        """
        if not user_obj.is_authenticated:
            return False

        # fetch all policies covering this action
        policies = Policy.objects.filter(
            actions__name=perm
        ).distinct()

        # bail if no policy covers it
        if not policies.exists():
            return False

        # load subject attrs into a dict
        subj_attrs = {
            (sa.name, sa.value)
            for sa in user_obj.abac_subject_attributes.all()
        }

        # fetch resource attrs
        if obj is not None:
            ct = ContentType.objects.get_for_model(obj)
            res_attrs = {
                (ra.name, ra.value)
                for ra in ct.abac_resource_attributes.filter(object_id=obj.pk)
            }
        else:
            res_attrs = set()

        # evaluate each policy
        for policy in policies:
            # check all subject requirements
            subj_req = {
                (r.name, r.value)
                for r in policy.subject_attribute_requirements.all()
            }
            if not subj_req.issubset(subj_attrs):
                continue

            # check resource requirements
            res_req = {
                (r.name, r.value)
                for r in policy.resource_attribute_requirements.all()
            }
            if not res_req.issubset(res_attrs):
                continue

            # if we reach here, policy matches -> enforce effect
            return policy.effect == "allow"

        # no matching allow policy found
        return False

