import { defineStore } from 'pinia'
import http from '@/utils/http'

export const useAuthStore = defineStore('auth', {
  state: () => ({
    user: null,
  }),
  getters: {
    isLoggedIn: (state) => !!state.user,
    username:   (state) => state.user?.username || '',
  },
  actions: {
    async fetchUser() {
      try {
        const { data } = await http.get('/api/user_self/');
        this.user = data;
      } catch (err) {

        this.user = null;
      }
    },
    logout() {
      this.user = null;
    }
  }
})

