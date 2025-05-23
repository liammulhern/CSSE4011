from rest_framework.routers import DefaultRouter

from supplychain.views.views_product import ProductViewSet
from supplychain.views.views_events import TrackerEventViewSet, ProductEventViewSet
from supplychain.views.views_productorder import ProductOrderViewSet
from supplychain.views.views_tracker import TrackerViewSet

router = DefaultRouter()
router.register(r'products', ProductViewSet, basename='product')
router.register(r'productorders', ProductOrderViewSet, basename='productorder')
router.register(r'trackerevents', TrackerEventViewSet, basename='trackerevent')
router.register(r'productevents', ProductEventViewSet, basename='productevent')
router.register(r'trackers', TrackerViewSet, basename='tracker')

urlpatterns = router.urls
