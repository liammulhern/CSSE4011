from django.contrib import admin
from django.contrib.contenttypes.admin import GenericTabularInline

from abac.models import SubjectAttribute
@admin.register(SubjectAttribute)
class SubjectAttributeAdmin(admin.ModelAdmin):
    list_display = ('user', 'name', 'value')
    list_filter = ('name',)
    search_fields = ('user__username', 'name', 'value')


from abac.models import ResourceAttribute
@admin.register(ResourceAttribute)
class ResourceAttributeAdmin(admin.ModelAdmin):
    list_display = ('content_object', 'name', 'value')
    list_filter = ('name',)
    search_fields = ('name', 'value')


from abac.models import PolicySubjectAttribute 
class PolicySubjectAttributeInline(admin.TabularInline):
    model = PolicySubjectAttribute
    extra = 1


from abac.models import PolicyResourceAttribute 
class PolicyResourceAttributeInline(admin.TabularInline):
    model = PolicyResourceAttribute
    extra = 1


from abac.models import PolicyAction 
class PolicyActionInline(admin.TabularInline):
    model = PolicyAction
    extra = 1


from abac.models import Policy
@admin.register(Policy)
class PolicyAdmin(admin.ModelAdmin):
    list_display = ('name', 'effect')
    list_filter = ('effect',)
    search_fields = ('name',)
    inlines = (
        PolicySubjectAttributeInline,
        PolicyResourceAttributeInline,
        PolicyActionInline,
    )
