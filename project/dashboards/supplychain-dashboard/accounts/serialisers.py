from rest_framework import serializers
from accounts.models import Company, UserCompany, UserRole, User

class CompanySerializer(serializers.ModelSerializer):
    class Meta:
        model = Company
        fields = ('id', 'name', 'type')


class UserCompanySerializer(serializers.ModelSerializer):
    company = CompanySerializer(read_only=True)

    class Meta:
        model = UserCompany
        fields = (
            'company',
            'assigned_at',
            'is_active',
        )


class UserRoleSerializer(serializers.ModelSerializer):
    role_name = serializers.CharField(source='role.name', read_only=True)
    company = CompanySerializer(source='role.company', read_only=True)

    class Meta:
        model = UserRole
        fields = (
            'role_name',
            'company',
            'assigned_at',
        )


class UserSerializer(serializers.ModelSerializer):
    companies = UserCompanySerializer(source='user_companies', many=True, read_only=True)
    roles     = UserRoleSerializer(source='user_roles',     many=True, read_only=True)

    class Meta:
        model  = User
        fields = (
            'id',
            'username',
            'first_name',
            'last_name',
            'email',
            'companies',
            'roles',
        )

