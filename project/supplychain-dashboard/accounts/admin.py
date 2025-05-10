from django.contrib import admin
from django.contrib.auth.admin import UserAdmin as BaseUserAdmin

from accounts.models import Company
@admin.register(Company)
class CompanyAdmin(admin.ModelAdmin):
    list_display = ('name', 'type', 'created_at')
    list_filter = ('type',)
    search_fields = ('name',)


from accounts.models import User
@admin.register(User)
class UserAdmin(BaseUserAdmin):
    fieldsets = (
        (None,               {'fields': ('username', 'password')}),
        ('Personal info',    {'fields': ('first_name', 'last_name', 'email')}),
        ('Company',          {'fields': ('company',)}),
        ('Permissions',      {'fields': ('is_active', 'is_staff', 'is_superuser',
                                         'groups', 'user_permissions')}),
        ('Important dates',  {'fields': ('last_login', 'date_joined')}),
    )

    list_display = ('username', 'email', 'company', 'is_staff', 'is_active')
    list_filter = ('company', 'is_staff', 'is_active', 'is_superuser', 'groups')
    search_fields = ('username', 'email')


from accounts.models import Role
@admin.register(Role)
class RoleAdmin(admin.ModelAdmin):
    list_display = ('name',)
    search_fields = ('name',)


from accounts.models import UserRole
@admin.register(UserRole)
class UserRoleAdmin(admin.ModelAdmin):
    list_display = ('user', 'role', 'assigned_at')
    list_filter = ('role', 'assigned_at')
    search_fields = ('user__username', 'role__name')

