from rest_framework.routers import DefaultRouter

from supplychain.views_product import ProductViewSet

router = DefaultRouter()
router.register(r'products', ProductViewSet, basename='product')

urlpatterns = router.urls
