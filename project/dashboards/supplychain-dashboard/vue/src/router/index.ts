import { createRouter, createWebHistory, type RouteMeta } from 'vue-router'
import { isAuthenticated } from '@/services/auth';
import { useAuthStore } from '@/stores/auth';
import DashboardLayoutVue from '@/layouts/dashboard.vue';

interface IRouteMeta {
  title: string
}

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'landing',
      component: () => import('@/views/Landing.vue'),
      meta: {
        title: 'Path Ledger',
        requiresAuth: false
      } as RouteMeta & IRouteMeta,
    },
    {
      path: '/login',
      name: 'login',
      component: () => import('@/views/Login.vue'),
      meta: {
        title: 'Login',
        requiresAuth: false
      } as RouteMeta & IRouteMeta,
    },
    {
      path: '/dashboard',
      component: DashboardLayoutVue,
      redirect: '/dashboard/home',
      name: 'dashboard',
      meta: {
        title: 'Dashboard',
      },
      children: [
        {
          path: 'home',
          name: 'home',
          component: () => import('@/views/dashboard/pages/home/Home.vue'),
          meta: {
            title: 'Home',
          } as RouteMeta & IRouteMeta
        },
        {
          path: 'products',
          name: 'products_index',
          component: () => import('@/views/dashboard/pages/products/Index.vue'),
          meta: {
            title: 'Products',
          } as RouteMeta & IRouteMeta,
        },
        {
          path: 'products/:id',
          component: () => import('@/views/dashboard/pages/products/product/Index.vue'),
          name: 'product_index',
          meta: {
            title: 'Product',
            hasQRAuth: true
          },
        },
        {
          path: 'product_types',
          name: 'product_types_index',
          component: () => import('@/views/dashboard/pages/product_types/Index.vue'),
          meta: {
            title: 'Catalog',
          } as RouteMeta & IRouteMeta,
        },
        {
          path: 'product_types/:id',
          component: () => import('@/views/dashboard/pages/product_types/type/Index.vue'),
          name: 'product_type_index',
          meta: {
            title: 'Type',
            hasQRAuth: true
          },
        },
        {
          path: 'product_orders',
          name: 'product_orders_index',
          component: () => import('@/views/dashboard/pages/product_orders/Index.vue'),
          meta: {
            title: 'Orders',
          } as RouteMeta & IRouteMeta,
        },
        {
          path: 'product_orders/:id',
          name: 'product_order_index',
          component: () => import('@/views/dashboard/pages/product_orders/order/Index.vue'),
          meta: {
            title: 'Order',
            hasQRAuth: true
          }
        },
        {
          path: 'tracker/:id',
          component: () => import('@/views/dashboard/pages/tracker/Index.vue'),
          name: 'tracker_index',
          meta: {
            title: 'Tracker',
            hasQRAuth: true
          },
        },
        {
          path: 'user',
          name: 'user_index',
          component: () => import('@/views/dashboard/pages/user/Index.vue'),
          meta: {
            title: 'User',
          } as RouteMeta & IRouteMeta
        },
        {
          path: 'settings',
          name: 'settings_index',
          component: () => import('@/views/dashboard/pages/settings/Index.vue'),
          meta: {
            title: 'Settings',
          } as RouteMeta & IRouteMeta
        },
      ],
    },
    {
      path: '/:pathMatch(.*)',
      name: 'not-found',
      component: () => import('@/views/404.vue'),
      meta: {
        title: 'Page Not Found',
      } as RouteMeta & IRouteMeta,
    },
  ]
});

router.beforeEach(async (to, from, next) => {
  document.title = to.meta.title as string;

  const auth = useAuthStore();
  const hasPublicAuth = to.matched.some(r => r.meta.requiresAuth === false)
  const hasQRAuth = to.matched.some(r => r.meta.hasQRAuth === true)

  // If we have a token but no user, try to fetch
  if (isAuthenticated() && !auth.user) {
    try {
      await auth.fetchUser();
    } catch {
      // ignore—fetchUser will clear user on failure
    }
  }

  if (isAuthenticated() || hasPublicAuth)
    return next();

  // already logged in and going to Login send them home
  if (to.name === 'login' && isAuthenticated())
    return next({ name: 'home' });

  if (to.query.token && hasQRAuth)
    return next();

  return next({ name: 'login', query: { redirect: to.fullPath } });
})

export default router
