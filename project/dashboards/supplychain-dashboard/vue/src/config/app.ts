interface IDashboardMenus {
  title: string
  icon: string
  path: string
  description?: string
  hidden?: boolean
}

export const SIDEBAR_EXPAND_WIDTH = 280;
export const SIDEBAR_COLLAPSED_WIDTH = 72;
export const APP_MENU: Record<string, { name: string, routes: IDashboardMenus[] }> = {
  main: {
    name: 'Core',
    routes: [
      {
        title: 'Dashboard',
        icon: 'LayoutDashboard',
        path: 'home',
      },
      {
        title: 'Products',
        icon: 'Package',
        path: 'products',
        description: 'Manage Manufactured Products'
      },
      {
        title: 'Product Orders',
        icon: 'ClipboardCheck',
        path: 'product_orders',
        description: 'Manage Product Orders'
      },
      {
        title: 'Trackers',
        icon: 'ClipboardCheck',
        path: 'trackers',
        description: 'Manage Trackers'
      },
    ],
  },
};

export const globalSearch = {

};
