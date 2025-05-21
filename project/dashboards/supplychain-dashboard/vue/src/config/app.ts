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
        title: 'Product Orders',
        icon: 'ClipboardCheck',
        path: 'product_orders',
        description: 'Manage Product Orders'
      },
    ],
  },
  settings: {
    name: 'General',
    routes: [
      {
        path: 'user',
        title: 'User',
        icon: 'Users',
        description: 'Manage User'
      },
      {
        path: 'settings',
        title: 'Settings',
        icon: 'Settings2',
        description: 'Settings'
      },
    ],
  },
};

export const globalSearch = {

};
