from django.contrib import admin
from django.contrib.auth import get_user_model
from django.contrib.auth.admin import UserAdmin as BaseUserAdmin

from .models import Company, UserCompany, Role, UserRole

User = get_user_model()


class UserCompanyInline(admin.TabularInline):
    model = UserCompany
    extra = 1
    fields = ("company", "is_active", "assigned_at")
    readonly_fields = ("assigned_at",)


class UserRoleInline(admin.TabularInline):
    model = UserRole
    extra = 1
    fields = ("role", "assigned_at")
    readonly_fields = ("assigned_at",)


class RoleInline(admin.TabularInline):
    model = Role
    extra = 1
    fields = ("name",)
    verbose_name = "Role"
    verbose_name_plural = "Roles"


@admin.register(Company)
class CompanyAdmin(admin.ModelAdmin):
    list_display    = ("name", "type", "created_at")
    list_filter     = ("type",)
    search_fields   = ("name",)
    readonly_fields = ("created_at",)
    inlines         = (RoleInline, UserCompanyInline)


@admin.register(Role)
class RoleAdmin(admin.ModelAdmin):
    list_display  = ("name", "company")
    list_filter   = ("company", "name")
    search_fields = ("company__name",)
    autocomplete_fields = ("company",)


@admin.register(UserCompany)
class UserCompanyAdmin(admin.ModelAdmin):
    list_display    = ("user", "company", "is_active", "assigned_at")
    list_filter     = ("company", "is_active")
    search_fields   = ("user__username", "company__name")
    readonly_fields = ("assigned_at",)
    autocomplete_fields = ("user", "company")


@admin.register(UserRole)
class UserRoleAdmin(admin.ModelAdmin):
    list_display    = ("user", "role", "assigned_at")
    list_filter     = ("role__company", "role__name")
    search_fields   = ("user__username", "role__name")
    readonly_fields = ("assigned_at",)
    autocomplete_fields = ("user", "role")


@admin.register(User)
class UserAdmin(BaseUserAdmin):
    inlines = (UserCompanyInline, UserRoleInline)

    list_display = (
        "username",
        "email",
        "first_name",
        "last_name",
        "is_staff",
        "is_active",
    )
    list_filter = (
        "is_staff",
        "is_active",
        "user_companies__company",
        "user_roles__role__company",
    )
    search_fields = ("username", "email", "first_name", "last_name")

