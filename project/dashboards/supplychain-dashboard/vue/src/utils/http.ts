import axios from 'axios'
import type {AxiosInstance} from 'axios';
import router from '@/router'
import { refreshToken } from '@/services/auth';

const http: AxiosInstance = axios.create({
  baseURL: import.meta.env.VITE_API_BASE_URL || 'http://localhost:8000',
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

http.interceptors.response.use(
  response => response,
  async error => {
    const { response, config: originalRequest } = error;
    const status = response?.status;

    // if it’s a 401 and we haven’t already retried this request:
    if (status === 401 && !originalRequest._retry) {
      originalRequest._retry = true;

      try {
        // try to get a new access token
        await refreshToken();
        // on success, the new token is in localStorage
        // retry the original request with the new token
        return http(originalRequest);
      } catch (refreshError) {
        // refresh also failed (e.g. refresh token expired) → redirect to login
        const returnTo = router.currentRoute.value.fullPath;
        router.push({ 
          name: 'login', 
          query: { redirect: returnTo } 
        });
      }
    }

    // for any other errors, or if we already retried, just reject
    return Promise.reject(error);
  }
)

export default http

