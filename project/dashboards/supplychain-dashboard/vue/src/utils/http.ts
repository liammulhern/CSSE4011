import axios from 'axios'
import type { AxiosInstance } from 'axios';
import router from '@/router'
import { refreshToken } from '@/services/auth';

const http: AxiosInstance = axios.create({
  baseURL: import.meta.env.VITE_API_BASE_URL || 'https://pathledger.live',
  // baseURL: import.meta.env.VITE_API_BASE_URL || 'http://localhost:8000',
  headers: {
    'Content-Type': 'application/json',
  },
})

http.interceptors.request.use(config => {
  const token = localStorage.getItem('access')

  if (token) {
    config.headers.Authorization = `Bearer ${token}`
  }

  return config
})

const REFRESH_PATH = '/api/token/refresh/'

http.interceptors.response.use(
  response => response,
  async error => {
    const { response, config: originalRequest } = error;
    const status = response?.status;
    const requestUrl = originalRequest?.url || ''

    if (status === 401 && !originalRequest._retry) {

      const current = router.currentRoute.value
      const hasQRAuth = current.matched.some(r => (r.meta as any).hasQRAuth === true)

      // if we're on a QR‐protected page, skip redirect and just reject
      if (hasQRAuth) {
        return Promise.reject(error)
      }

      originalRequest._retry = true;

      try {
        await refreshToken();

        return http(originalRequest);
      } catch (refreshError) {
        localStorage.removeItem('refresh');
        localStorage.removeItem('access');

        router.push({
          name: 'login',
        });
      }
    }

    // all other errors: just reject
    return Promise.reject(error);
  }
);

export default http
