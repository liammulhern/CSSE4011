from django.urls import path
from accounts.views import current_user, create_temporary_user

urlpatterns = [
    path('user_self/', current_user, name='current_user'),
    path('create_temporary_user/', create_temporary_user, name='create_temporary_user'),
]
